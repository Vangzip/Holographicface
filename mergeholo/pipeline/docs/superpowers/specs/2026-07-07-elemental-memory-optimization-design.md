# Elemental 内存路径优化设计

## 目标

优化 `all` 流程里的 `elemental` 阶段：该阶段直接接收 `multiview` 已经生成在内存中的帧数据，并在目标运行环境中把 elemental 内存转换控制到 2 秒以内。图片数据在这条路径上全程不落盘。

这个目标不包含 `--stage elemental` 单独从 72,900 张 JPG 文件读取的耗时。

## 当前行为

默认配置为：

- `multiview_angle=90`
- `multiview_per=3`
- `multiview_resolution=150`
- `target_rows=150`
- `target_cols=150`

对应数据规模：

- 输入视角数：`270 * 270 = 72,900`
- 每个视角帧：`150 * 150 * 3 = 67,500` 字节
- elemental 输出图数量：`150 * 150 = 22,500`
- 每张 elemental 输出图：`270 * 270 * 3 = 218,700` 字节
- elemental 输出总内存：`4,920,750,000` 字节，约 `4.58 GiB`

旧实现按 target pixel 外层、view 内层生成输出。内存路径中，每次只从源帧取 3 字节，但相邻读取会跨过一个完整源帧，即约 `67,500` 字节。这个访问模式缓存局部性很差，并且默认只用单线程执行约 16.4 亿次 RGB 像素搬运。

旧实现还会在每张输出图写入前整图清零；当源尺寸和 target 尺寸完全匹配时，所有输出字节都会被覆盖，这次清零属于额外写入。

## 范围

本次优化包含：

- 只优化 `all` 流程中 `MultiviewMemoryResult` 已经持有 `MemoryFrameSink` 和 `MultiviewRenderPlan` 的内存输入路径。
- 保持图片数据全程在内存中。
- 暂时保留 `ElementalMemoryResult` 的连续物理输出 buffer，不改成 lazy/strided view。
- 保持既有输出顺序和 RGB 字节顺序。
- 保持 `elementalFlipSourceY` 和 `elementalFlipViewRows` 语义。
- 文件/JPG 输入路径保持兼容，但不纳入 2 秒验收目标。

本次不包含：

- 优化 standalone `--stage elemental` 的 JPG 解码或文件系统读取。
- 把 `ElementalMemoryResult` 改成懒加载视图。
- 重写 multiview 渲染或 GPU readback 布局。
- 在当前 Mac 工作区证明完整 Windows/OSG 全流程运行时。

## 架构设计

新增一个纯 C++ 的 elemental 内存转换边界，不依赖 OpenCV、OSG、Qt 或文件系统状态。`processElemental()` 继续负责校验、日志、分配和文件模式兼容；内存路径把核心布局转换交给 helper。

转换关系为：

```text
source[viewRow][viewCol][targetRow][targetCol][rgb]
  -> output[targetRow][targetCol][outputViewRow][outputViewCol][rgb]
```

helper 接收尺寸、字节 stride、翻转标记、源行方向、源指针、目标指针和线程数。`all` 内存路径把源行标记为 OpenGL bottom-up，因为 multiview 内存帧来自 raw `glReadPixels`；旧 JPG 路径在写文件前做过 `flipVertical()`，所以这里必须显式处理源行方向，才能保持两条路径的行语义一致。

## 优化策略

实现策略：

- 将 target 像素范围按连续区间切分给多个线程。
- 每个线程写入互不重叠的 elemental 输出图范围，避免数据竞争。
- 源数据只读，输出按 target image 分片写入。
- 源尺寸完全覆盖 target 时不做无意义的整图清零。
- 源尺寸小于 target 时保留 zero padding 行为。
- `elementalWriterThreads` 在内存路径中生效；配置为 `0` 或负数时，按硬件并发数保守选择线程数。

文件路径继续保留原有单线程 cache 逻辑，避免这次优化扩散到非目标路径。

## 必须保持的语义

测试需要锁住：

- 输出元数据：`imageCount`、`imageBytes`、`totalBytes`、`rows`、`cols`
- `(targetRow, targetCol, outputViewRow, outputViewCol)` 到源帧和源像素的映射
- `elementalFlipSourceY`
- `elementalFlipViewRows`
- OpenGL bottom-up 内存源行与旧 JPG `flipVertical()` 后行语义一致
- 单线程和多线程输出 byte-identical
- 源帧小于 target 网格时的 zero padding 行为

## 测试计划

当前 `pipeline` 目录没有独立构建系统，本机 Mac 环境也不能证明 Windows/OSG 集成。因此先用直接 `clang++` 编译的纯 C++ 测试锁住核心内存转换。

测试覆盖：

- 小尺寸合成数据，每个源像素使用唯一 RGB 值
- source Y flip
- view row flip
- OpenGL bottom-up 源行
- 单线程和多线程一致性
- 中等规模纯内存 benchmark

最终 `<2s` 验收必须在目标 Windows/OSG 环境里跑 `all` 流程并读取 pipeline timing 日志。

## 风险

- 物理输出 buffer 仍然需要写入约 `4.58 GiB`，目标机器可能受内存带宽限制。
- 当前 Mac 工作区缺少完整 Windows/OSG 构建环境，只能验证纯 C++ helper 和局部接线。
- 如果目标机器仍超过 2 秒，下一步应考虑把 `ElementalMemoryResult` 改成 lazy/strided view，或在 multiview 阶段更早生成 elemental-major 布局。
- 目标 Windows 工程必须纳入新增的 `elemental/ElementalMemoryTransform.cpp`，否则会链接失败或运行不到新路径。

## 验收标准

- `all` 流程中，图片数据从 multiview 输出到 elemental 输出都留在内存中。
- memory-input elemental 转换不读 JPG、不创建重复 view cache。
- 小尺寸合成测试证明输出字节符合参考映射。
- 单线程和多线程输出完全一致。
- `-DNDEBUG` 编译下测试不会因为 `assert` 被移除而假通过。
- 目标运行环境日志显示 `[elemental] stored output images in memory in <2.000s`。
- standalone JPG `--stage elemental` 路径保持兼容，但不要求达到 2 秒。

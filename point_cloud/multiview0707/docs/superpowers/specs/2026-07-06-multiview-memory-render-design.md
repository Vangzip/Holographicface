# 多视角内存渲染设计

## 目标

在参数 `multiview_angle=90`、`multiview_pre=3`、`multiview_resolution=150` 下，当前完整运行耗时为 `2469.518s`。目标是在保留全部 `72,900` 个视角输出的前提下，把输出从逐张 JPEG 落盘改为连续 raw RGB 内存输出，最终在真实 Windows/OSG 环境中验证端到端耗时小于 `10s`。

## 当前证据

当前源码是文件输出导向：

- `main.cpp` 解析 `-angle`、`-pre`/`-per`、`-resolution` 参数，并在旧路径中给相机安装 `CaptureDrawCallback`。
- `modelMoveHandler.cpp` 用 `m_angle * m_per` 同时作为纵向和横向循环上限。
- 当 `angle=90`、`per=3` 时，每个方向都是 `270` 个采样点，总输出数为 `270 * 270 = 72,900`。
- 旧逻辑每张图需要等待多帧事件、深拷贝 `osg::Image`、垂直翻转，再通过 `osgDB::writeImageFile` 写 JPEG。
- 原始 `Release|x64` 工程配置中编译优化关闭，这会进一步拖慢运行。

`2469.518s` 折算为每张约 `33.9ms`。如果仍然保留 `72,900` 次独立 JPEG 编码和写盘，要达到 `10s` 等价于每张约 `0.137ms`，不具备现实可行性。因此输出契约必须改为内存输出。

首轮 memory 路径实测仍为约 `1218.39s`，关键计时如下：

- `render_call_seconds=1215.48`
- `render_seconds=927.688`
- `readback_seconds=287.388`
- `process_seconds=1218.39`

`1215.48 / 72,900 = 0.01667s`，基本等于 60Hz 一帧时间，说明当前 memory 路径仍被窗口双缓冲/帧交换节流。下一步必须让 memory 模式使用离屏 pbuffer、单缓冲、无窗口装饰，并显式设置 read/draw buffer，避免 72,900 次 `viewer->frame()` 被显示器刷新率限制。

## 输出契约

优化路径将所有视角保存到一块连续 raw RGB 字节缓冲区。

- 图像格式：raw RGB，每通道 8 位。
- 单张尺寸：`resolution x resolution`。
- 帧数：`(angle * pre) * (angle * pre)`。
- 缓冲区大小：`frame_count * resolution * resolution * 3`。
- 目标参数下大小：`72,900 * 150 * 150 * 3 = 4,920,750,000` 字节。
- 帧顺序：按行优先保存，对齐旧逻辑中文件名的逻辑顺序；`height` 是外层循环，`rotate` 是内层循环。
- 磁盘输出：优化路径不写 `72,900` 张图；如需人工检查，只保留可选抽样导出。

本设计不保存 `72,900` 个独立 `osg::Image` 或 `cv::Mat` 对象，避免大量对象分配和管理开销。
`MemoryFrameSink` 使用未初始化的连续字节分配，避免对约 `4.92GB` 的目标 buffer 做无意义清零；后续 `glReadPixels` 会覆盖每个 frame slot。

## 渲染设计

用显式批处理渲染器替代事件驱动截图循环。

批处理流程：

1. 根据 `angle`、`pre`、`resolution` 创建 `MultiviewRenderPlan`。
2. 预分配足够容纳全部 raw RGB 帧的 `MemoryFrameSink`。
3. 初始化并 realize OSG viewer。
4. 按旧逻辑顺序更新模型旋转。
5. 渲染一帧。
6. 在相机 post-draw callback 中把像素直接读入当前帧 slot。
7. 记录渲染耗时、readback 耗时、渲染循环耗时和端到端耗时。

旧逻辑在每次捕获前等待多帧。优化路径默认不等待额外帧；如果目标模型确实需要稳定帧，可后续增加可配置 warmup 帧数，并单独计入耗时。

内存 readback 必须在 `glReadPixels` 前设置 `GL_PACK_ALIGNMENT=1`。目标分辨率下每行 RGB 字节数是 `150 * 3 = 450`，不是 OpenGL 默认 4 字节对齐；如果不设置，OpenGL 可能按 452 字节 stride 写入，导致越过当前 frame slot。

## 组件

### `MultiviewRenderPlan`

纯 C++ 组件，不依赖 OSG。

职责：

- 校验 `angle > 0`、`pre > 0`、`resolution > 0`。
- 用显式溢出检查计算 `samples_per_axis = angle * pre`，避免在参数异常大时先发生 `int` 溢出。
- 计算 `frame_count = samples_per_axis * samples_per_axis`。
- 计算 `step_degrees = 1.0 / pre`。
- 将 frame index 映射到 row、column、角度和 byte offset。
- 保持现有行优先遍历语义。

### `MemoryFrameSink`

纯 C++ 组件，不依赖 OSG。

职责：

- 分配连续 raw RGB 缓冲区。
- 通过 `unsigned char* frameData(frame_index)` 暴露当前帧写入地址。
- 拒绝越界 frame index。
- 汇报总字节数和单帧字节数。
- 避免逐帧堆分配。

### `MultiviewBatchRenderer`

OSG 相关组件。

职责：

- 管理高速批处理渲染循环。
- 复用 `modelMoveHandler` 初始化后的模型 transform。
- 按旧逻辑累计执行列旋转和换行旋转。
- 不依赖鼠标事件触发。
- memory 模式不安装旧的统计、窗口、截图和交互 event handler，避免这些 handler 在 `72,900` 次 `viewer->frame()` 中增加无关开销。
- memory 模式强制 `osgViewer::Viewer::SingleThreaded`，降低线程调度开销，并让 post-draw readback 与当前图形上下文关系更明确。
- memory 模式使用离屏 pbuffer 图形上下文：`windowDecoration=false`、`doubleBuffer=false`、`pbuffer=true`、`vsync=false`。
- memory 模式相机显式设置 `drawBuffer/readBuffer=GL_FRONT`；旧 JPEG 模式继续使用可见窗口和 `GL_BACK`。
- 在 post-draw callback 中用 `GL_PACK_ALIGNMENT=1` 读像素到 `MemoryFrameSink`。
- 同时保存并重置 `GL_PACK_ROW_LENGTH`、`GL_PACK_SKIP_ROWS`、`GL_PACK_SKIP_PIXELS`，避免上游 OpenGL pack state 影响连续 raw RGB slot。
- 统计实际成功 readback 的 `framesCaptured`；只有 `framesCaptured == frame_count` 且 `readbackErrors == 0` 时，才把 `bytesCaptured` 标记为有效。
- 输出性能计数和 readback 错误计数。
- 缓存模型旋转中心，避免 72,900 帧循环中反复调用 `getBound().center()`。

### 旧路径兼容

旧 JPEG 事件路径保留在显式 `-output legacy-jpg` 模式下。默认性能路径是 `-output memory`，不写 `72,900` 张 JPEG。

## 测试策略

按 TDD 先写测试再实现。

纯单元测试：

- `MultiviewRenderPlan` 在 `90,3,150` 下返回 `270` 个轴向采样、`72,900` 帧。
- `MultiviewRenderPlan` 对第一帧、换行帧、最后一帧返回确定的 row/column/offset。
- `MultiviewRenderPlan` 拒绝可能造成 `samples_per_axis`、`frame_count` 或 `totalBytes` 溢出的异常参数。
- `MemoryFrameSink` 对目标参数报告 `4,920,750,000` 字节。
- `MemoryFrameSink::frameData` 返回连续 slot，并拒绝越界访问。
- `MultiviewGraphicsConfig` 在 memory 模式返回 pbuffer、单缓冲、无窗口装饰、禁用 vsync、前缓冲读写；legacy 模式保留可见双缓冲窗口。

集成和性能测试：

- 输出总帧数、总字节数、readback 错误数、渲染耗时、readback 耗时、渲染循环耗时、memory 分支端到端耗时和从参数校验后开始的整体处理耗时。
- 输出实际成功捕获帧数 `multiview_memory_frames_captured`，并要求它等于 `72,900`。
- 在 Windows + OSG 依赖完整的环境中运行目标参数，验证端到端耗时小于 `10s`。
- 验证帧数严格等于 `72,900`。
- 验证默认 memory 路径不写 `72,900` 个文件。
- 只有当 `multiview_readback_errors=0` 时，`multiview_memory_bytes` 才可作为有效输出字节数证据。

当前 Mac 工作区缺少 Visual Studio、OSG、base 等真实构建依赖，因此最终运行验收必须在 `multiview.vcxproj` 指向的 Windows 依赖环境中完成。

## 验收标准

- `multiview_angle=90`、`multiview_pre=3`、`multiview_resolution=150` 产生 `72,900` 个内存 raw RGB 帧。
- 默认优化路径不写 `72,900` 张磁盘图片。
- 输出缓冲区连续，并可按 frame index 定位。
- `multiview_readback_errors=0`。
- `multiview_memory_frames_captured=72,900`。
- `multiview_graphics_pbuffer=1`、`multiview_graphics_double_buffer=0`、`multiview_graphics_vsync=0`、`multiview_graphics_window_decoration=0`。
- 输出足够的计时字段，能区分渲染、readback、渲染循环和端到端耗时。
- 在真实 Windows/OSG 环境中优先验证 `multiview_process_seconds < 10.0`；同时记录 `multiview_end_to_end_seconds` 用于拆分 buffer 分配和渲染阶段。

## 风险

- raw RGB 缓冲区约 `4.92GB`，进程必须是 x64，并且机器要有足够内存余量。
- 同步 `glReadPixels` 仍可能造成 GPU 管线阻塞。如果首版 memory 路径仍超过 `10s`，下一步应改为 PBO 异步 readback 或多缓冲 readback。
- 如果模型本身很重，渲染时间可能成为主要瓶颈；现有计时字段用于判断是否需要继续优化渲染状态和场景结构。
- raw RGB 默认是 OpenGL readback 的像素方向，不做旧 JPEG 路径里的逐帧 `flipVertical()`；如业务要求完全同向，需要把翻转作为可选步骤并单独计时。

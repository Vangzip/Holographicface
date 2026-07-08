# Elemental 内存路径优化实施记录

> 说明：本文件记录已经执行的实施方案、验证结果和目标机验收步骤。代码标识符、文件名、编译命令和日志关键字保持英文，以避免破坏现有工程接口。

## 目标

让 `all` 流程中的 `elemental` 阶段直接消费 `multiview` 内存帧，并在目标运行环境中把 elemental 内存转换控制到 2 秒以内。

## 方案摘要

- 新增纯 C++ helper：`elemental/ElementalMemoryTransform.h` 和 `elemental/ElementalMemoryTransform.cpp`
- helper 不依赖 OpenCV、OSG、Qt 或文件系统。
- `processElemental()` 继续负责参数校验、日志、内存分配和文件路径兼容。
- `all` 内存路径调用 `storeElementalFromMemory()` 完成布局转换。
- 文件/JPG 输入路径保持旧逻辑，不作为 2 秒目标。
- 内存源帧按 OpenGL bottom-up 处理，保持和旧 JPG `flipVertical()` 后路径的行语义一致。
- 测试不再依赖 `assert`，避免 `-DNDEBUG` 下假通过。

## 文件变更

新增：

- `elemental/ElementalMemoryTransform.h`
- `elemental/ElementalMemoryTransform.cpp`
- `tests/test_elemental_memory_transform.cpp`
- `docs/superpowers/specs/2026-07-07-elemental-memory-optimization-design.md`
- `docs/superpowers/plans/2026-07-07-elemental-memory-optimization.md`

修改：

- `elemental/ElementalProcessor.cpp`

## 关键实现点

### 纯内存转换 API

`ElementalMemoryTransformConfig` 描述转换所需的全部参数：

- view 网格尺寸
- source 图像尺寸
- target 图像尺寸
- `flipSourceY`
- `flipViewRows`
- `sourceRowsBottomUp`
- `threadCount`

`storeElementalFromMemory()` 执行转换：

```text
source[viewRow][viewCol][targetRow][targetCol][rgb]
  -> output[targetRow][targetCol][outputViewRow][outputViewCol][rgb]
```

### 多线程策略

- 按 target image 连续范围切分任务。
- 每个线程写互不重叠的 output 区间。
- source 只读。
- 线程创建失败时，等待已启动线程结束，然后回退到单线程完整转换。

### Y 方向修正

旧 JPG 路径在写文件前调用 `flipVertical()`。新的 `all` 内存路径接收的是 raw `glReadPixels` 帧，行方向是 OpenGL bottom-up。

因此 `processElemental()` 调用 helper 时设置：

```cpp
transformConfig.sourceRowsBottomUp = true;
```

这样默认 `elementalFlipSourceY=true` 时，内存路径和旧 JPG 路径的逻辑行语义保持一致。

## TDD 执行记录

1. 先创建 `tests/test_elemental_memory_transform.cpp`。
2. 首次编译失败，原因是 `ElementalMemoryTransform.cpp` 尚不存在，红灯符合预期。
3. 添加 `ElementalMemoryTransform.h/.cpp`。
4. 运行测试通过。
5. 接入 `ElementalProcessor.cpp` 内存路径。
6. 添加默认生产尺寸计算测试。
7. 添加中等规模 benchmark。
8. 代码评审发现两个问题：
   - raw OpenGL 内存帧 Y 方向和旧 JPG 路径不一致。
   - 测试使用 `assert`，`-DNDEBUG` 下会假通过。
9. 增加 `sourceRowsBottomUp` 并补充 bottom-up 测试。
10. 把测试断言改成显式 `expect()` 失败退出。

## 本地验证

当前 Mac 工作区可验证纯 C++ helper，但不能证明完整 Windows/OSG pipeline。

已通过：

```bash
clang++ -std=c++17 -Wall -Wextra -Werror -O2 -pthread \
  tests/test_elemental_memory_transform.cpp \
  elemental/ElementalMemoryTransform.cpp \
  -o /tmp/test_elemental_memory_transform_werror \
  && /tmp/test_elemental_memory_transform_werror
```

已通过：

```bash
clang++ -std=c++17 -Wall -Wextra -Werror -O3 -DNDEBUG -pthread \
  tests/test_elemental_memory_transform.cpp \
  elemental/ElementalMemoryTransform.cpp \
  -o /tmp/test_elemental_memory_transform_ndebug \
  && /tmp/test_elemental_memory_transform_ndebug
```

已通过：

```bash
clang++ -std=c++17 -O2 -pthread \
  -c elemental/ElementalMemoryTransform.cpp \
  -o /tmp/ElementalMemoryTransform.o
```

最近一次中等规模 benchmark 输出：

```text
medium_benchmark_seconds=0.130624, threads=8, bytes=223948800
```

这个 benchmark 只代表当前 Mac 上的纯内存 helper 吞吐，不能替代目标 Windows/OSG 全流程验收。

## 当前本机限制

尝试单独编译 `ElementalProcessor.cpp`：

```bash
clang++ -std=c++17 -O2 -pthread $(pkg-config --cflags opencv4) \
  -I. -I../multiview\(1\) \
  -c elemental/ElementalProcessor.cpp \
  -o /tmp/ElementalProcessor.o
```

失败原因：

```text
fatal error: 'ModelMoveCameraConfig.h' file not found
```

这说明当前 Mac 快照不是完整目标工程环境。最终编译必须在目标 Windows/OSG 工程中完成。

## 目标机验收步骤

1. 确认目标 Windows 工程包含并编译：

```text
elemental/ElementalMemoryTransform.cpp
```

2. 用 Release x64 构建目标工程。

3. 运行正常 `all` 流程，配置应包含：

```ini
run_multiview=true
run_elemental=true
multiview_angle=90
multiview_per=3
multiview_resolution=150
target_rows=150
target_cols=150
```

4. 日志需要出现：

```text
[multiview] frames captured: 72900/72900
[elemental] input views: 270x270, each view: 150x150 from memory
[elemental] using multiview memory buffer directly; no file load or duplicate view cache.
[elemental] stored output images in memory in <2.000s
```

## 剩余风险

- 目标工程如果没有纳入新增 `.cpp`，会链接失败或运行不到新路径。
- 如果 standalone `--stage elemental` 仍要求输出文件集，这属于既有契约问题；本次需求明确不优化、不扩展该路径。
- 如果目标机仍超过 2 秒，应优先考虑 lazy/strided `ElementalMemoryResult` 或在 multiview 阶段生成 elemental-major 布局，而不是继续堆叠小循环优化。

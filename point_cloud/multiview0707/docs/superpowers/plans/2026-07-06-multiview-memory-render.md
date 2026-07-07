# 多视角内存渲染实施计划

> **给 agentic workers：** 实施本计划时使用 `superpowers:executing-plans` 或 `superpowers:subagent-driven-development`。步骤用 checkbox 跟踪。

**目标：** 在 `angle=90`、`pre=3`、`resolution=150` 下，把 `72,900` 个多视角输出写入连续 raw RGB 内存，默认不再逐帧写 JPEG，向端到端 `10s` 验收目标推进。

**架构：** 把逻辑拆成两层：纯 C++ 的计划和内存存储组件，以及 OSG 相关的批处理渲染组件。纯 C++ 组件可在当前 Mac 工作区测试；OSG 渲染和最终性能必须在 Windows/OSG 环境验证。

**技术栈：** C++17 标准库、Visual Studio 工程、OSG/OpenGL、简单 assert 单元测试。

---

## 文件结构

- 新建 `multiviewRenderPlan.h`：多视角采样计划、帧数、offset 计算。
- 新建 `multiviewRenderPlan.cpp`：`MultiviewRenderPlan` 实现。
- 新建 `memoryFrameSink.h`：连续 raw RGB 缓冲区接口。
- 新建 `memoryFrameSink.cpp`：`MemoryFrameSink` 实现。
- 新建 `multiviewGraphicsConfig.h`：memory/legacy 图形上下文策略。
- 新建 `multiviewGraphicsConfig.cpp`：pbuffer、单缓冲、读写 buffer 配置实现。
- 新建 `test_multiview_plan.cpp`：不依赖 OSG 的单元测试。
- 修改 `modelMoveHandler.h`：暴露已初始化的 `osg::MatrixTransform`，供批处理路径复用。
- 修改 `modelMoveHandler.cpp`：实现 `modelTransform()`，不改变旧 JPEG 行为。
- 新建 `multiviewBatchRenderer.h`：OSG 批处理渲染接口。
- 新建 `multiviewBatchRenderer.cpp`：批处理循环、post-draw readback、性能计时。
- 修改 `main.cpp`：默认 `-output memory`，显式 `-output legacy-jpg` 才走旧写盘路径。
- 修改 `multiview.vcxproj`：加入新文件，开启 `Release|x64` 的 `MaxSpeed` 优化。
- 修改 `multiview.vcxproj.filters`：让新文件在 VS 中归类显示。

## 任务 1：为采样计划写红灯测试

**文件：**

- 新建：`test_multiview_plan.cpp`
- 新建：`multiviewRenderPlan.h`

**步骤：**

- [x] 写 `test_multiview_plan.cpp`，验证目标参数下：
  - `samplesPerAxis() == 270`
  - `frameCount() == 72900`
  - `frameBytes() == 67500`
  - `totalBytes() == 4920750000ULL`
  - 行优先 index 映射正确
  - 非法参数和越界 frame 被拒绝
- [x] 写只有声明的 `multiviewRenderPlan.h`。
- [x] 运行：

```bash
clang++ -std=c++17 test_multiview_plan.cpp multiviewRenderPlan.cpp -o /tmp/test_multiview_plan
```

**预期：** 编译失败，因为 `multiviewRenderPlan.cpp` 还不存在。这是 TDD 红灯。

## 任务 2：实现 `MultiviewRenderPlan`

**文件：**

- 修改：`multiviewRenderPlan.h`
- 新建：`multiviewRenderPlan.cpp`
- 测试：`test_multiview_plan.cpp`

**步骤：**

- [x] 增加字段：`angle_`、`pre_`、`resolution_`、`samplesPerAxis_`。
- [x] 实现：
  - `angle()`
  - `pre()`
  - `resolution()`
  - `samplesPerAxis()`
  - `channels()`
  - `stepDegrees()`
  - `frameCount()`
  - `frameBytes()`
  - `totalBytes()`
  - `frameAt()`
- [x] 增加构造阶段的乘法溢出检查，拒绝异常大的 `angle * pre`、`frame_count` 和 `totalBytes`。
- [x] 运行：

```bash
clang++ -std=c++17 test_multiview_plan.cpp multiviewRenderPlan.cpp -o /tmp/test_multiview_plan && /tmp/test_multiview_plan
```

**预期：** 输出 `multiview plan tests passed`。

## 任务 3：为连续内存输出写红灯测试

**文件：**

- 修改：`test_multiview_plan.cpp`
- 新建：`memoryFrameSink.h`

**步骤：**

- [x] 增加 `MemoryFrameSink` 测试：
  - 不分配 storage 时仍能报告布局信息。
  - 分配 storage 后，`frameData(1) == frameData(0) + frameBytes()`。
  - 最后一帧 offset 正确。
  - 越界 frame index 被拒绝。
- [x] 写只有声明的 `memoryFrameSink.h`。
- [x] 运行：

```bash
clang++ -std=c++17 test_multiview_plan.cpp multiviewRenderPlan.cpp memoryFrameSink.cpp multiviewGraphicsConfig.cpp -o /tmp/test_multiview_plan
```

**预期：** 编译失败，因为 `memoryFrameSink.cpp` 还不存在。这是 TDD 红灯。

## 任务 4：实现 `MemoryFrameSink`

**文件：**

- 修改：`memoryFrameSink.h`
- 新建：`memoryFrameSink.cpp`
- 测试：`test_multiview_plan.cpp`

**步骤：**

- [x] 用 `new unsigned char[...]` 承载未初始化的连续 raw RGB 缓冲区，避免 `std::vector::resize()` 对约 `4.92GB` buffer 做清零。
- [x] 实现：
  - `frameCount()`
  - `frameBytes()`
  - `totalBytes()`
  - `data()`
  - `frameData()`
- [x] 防止 32 位平台无法承载超大 buffer。
- [x] 运行：

```bash
clang++ -std=c++17 test_multiview_plan.cpp multiviewRenderPlan.cpp memoryFrameSink.cpp multiviewGraphicsConfig.cpp -o /tmp/test_multiview_plan && /tmp/test_multiview_plan
```

**预期：** 输出 `multiview plan tests passed`。

## 任务 5：加入 OSG 批处理渲染器

**文件：**

- 新建：`multiviewBatchRenderer.h`
- 新建：`multiviewBatchRenderer.cpp`

**步骤：**

- [x] 定义 `MultiviewBatchStats`：
  - `framesRendered`
  - `framesCaptured`
  - `bytesCaptured`
  - `readbackErrors`
  - `renderSeconds`
  - `readbackSeconds`
  - `totalSeconds`
- [x] 定义 `MultiviewBatchRenderer`。
- [x] 在 `renderAll()` 中遍历 `samplesPerAxis * samplesPerAxis`。
- [x] 复用旧逻辑的旋转顺序：
  - 每列 `rotateZ(stepDegrees)`
  - 每行结束后 `rotateZ(-angle/2)`、`rotateX(-stepDegrees)`、`rotateZ(-angle/2)`
- [x] 用 camera post-draw callback 做 readback，避免在 `viewer->frame()` 返回后读错上下文。
- [x] 在 readback 前设置 `GL_PACK_ALIGNMENT=1`，避免 `150 * 3 = 450` 行宽被 OpenGL 默认 4 字节对齐补齐。
- [x] 保存并重置 `GL_PACK_ROW_LENGTH`、`GL_PACK_SKIP_ROWS`、`GL_PACK_SKIP_PIXELS`，避免继承上游 pack state。
- [x] 统计实际成功捕获帧数，只有 `framesCaptured == frameCount` 且 `readbackErrors == 0` 时，`bytesCaptured` 才为有效总字节数。
- [x] 统计 `glReadPixels` 错误；如果 `readbackErrors != 0`，不要把 `bytesCaptured` 当成有效输出字节数。
- [x] 缓存旋转中心，避免每次 `rotateZ/rotateX` 都调用 `modelTransform_->getBound().center()`。

## 任务 6：接入默认 memory 快路径

**文件：**

- 修改：`main.cpp`
- 修改：`modelMoveHandler.h`
- 修改：`modelMoveHandler.cpp`

**步骤：**

- [x] 增加 `-output` 参数。
- [x] 增加 `-pre` 作为 `-per` 的别名，匹配用户侧 `multiview_pre` 说法。
- [x] 初始化并校验 `angle`、`pre`、`resolution`，避免缺参时使用未初始化数值。
- [x] 默认 `outputMode = "memory"`。
- [x] memory 模式设置 `viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded)`。
- [x] 图形上下文设置 `traits->vsync = false`，避免批处理渲染被显示器刷新率限制。
- [x] memory 模式使用离屏 pbuffer、单缓冲、无窗口装饰，切断 60Hz 窗口帧交换节流。
- [x] memory 模式相机设置 `drawBuffer/readBuffer=GL_FRONT`；legacy 模式保留 `GL_BACK`。
- [x] memory 模式输出图形上下文诊断：
  - `multiview_graphics_pbuffer`
  - `multiview_graphics_double_buffer`
  - `multiview_graphics_vsync`
  - `multiview_graphics_window_decoration`
- [x] 显式 `-output legacy-jpg` 时才：
  - 安装旧统计、窗口、截图和交互 event handler
  - 分配旧截图路径使用的 `osg::Image`
  - 安装旧 `CaptureDrawCallback`
  - 挂载旧 `modelMoveHandler` 事件 handler
  - 进入旧 `while (!viewer->done()) viewer->frame()` 循环
- [x] memory 模式创建：
  - `MultiviewRenderPlan`
  - `MemoryFrameSink`
  - `MultiviewBatchRenderer`
- [x] memory 模式输出：

```text
multiview_memory_frames=...
multiview_memory_frames_captured=...
multiview_memory_bytes=...
multiview_readback_errors=...
multiview_render_seconds=...
multiview_readback_seconds=...
multiview_total_seconds=...
multiview_end_to_end_seconds=...
multiview_process_seconds=...
```

- [x] 在 `modelMoveHandler` 中增加 `modelTransform()`，让批处理路径复用模型居中、初始角度和相机设置。
- [x] 图形上下文创建失败时输出明确错误并返回非零退出码。
- [x] 模型加载为空时返回非零退出码，避免空场景输出假阳性。
- [x] memory 路径捕获 `std::exception`，输出 `multiview_error=...`。

## 任务 7：更新 Visual Studio 工程

**文件：**

- 修改：`multiview.vcxproj`
- 修改：`multiview.vcxproj.filters`

**步骤：**

- [x] 把以下文件加入工程：
  - `multiviewRenderPlan.h`
  - `multiviewRenderPlan.cpp`
  - `memoryFrameSink.h`
  - `memoryFrameSink.cpp`
  - `multiviewBatchRenderer.h`
  - `multiviewBatchRenderer.cpp`
  - `multiviewGraphicsConfig.h`
  - `multiviewGraphicsConfig.cpp`
- [x] 把 `Release|x64` 的 `<Optimization>` 改为 `MaxSpeed`。
- [x] 保持 Debug 配置为 `Disabled`，避免误改调试配置。

## 任务 8：本地验证

**文件：**

- 测试：`test_multiview_plan.cpp`
- 静态检查：`main.cpp`、`modelMoveHandler.cpp`、`multiviewBatchRenderer.cpp`、`multiview.vcxproj`

**步骤：**

- [x] 运行纯 C++ 测试：

```bash
clang++ -std=c++17 test_multiview_plan.cpp multiviewRenderPlan.cpp memoryFrameSink.cpp multiviewGraphicsConfig.cpp -o /tmp/test_multiview_plan && /tmp/test_multiview_plan
```

**实际结果：**

```text
multiview plan tests passed
```

- [x] 增加并验证溢出防御测试：
  - `MultiviewRenderPlan(std::numeric_limits<int>::max(), 2, 1)` 抛出 `std::length_error`。
  - `MultiviewRenderPlan(90000, 90000, 150)` 抛出 `std::length_error`。
- [x] 增加并验证图形上下文配置测试：
  - memory 模式：`pbuffer=true`、`doubleBuffer=false`、`windowDecoration=false`、`vsync=false`、前缓冲读写。
  - legacy 模式：`pbuffer=false`、`doubleBuffer=true`、`windowDecoration=true`、`vsync=false`、后缓冲读写。

- [x] 运行纯 C++ 语法检查：

```bash
clang++ -std=c++17 -fsyntax-only multiviewRenderPlan.cpp memoryFrameSink.cpp multiviewGraphicsConfig.cpp test_multiview_plan.cpp
```

**实际结果：** 退出码为 `0`。

- [x] 静态检查默认路径不写 JPEG：

```bash
rg -n "writeImageFile|legacy-jpg|outputMode|MultiviewBatchRenderer" main.cpp modelMoveHandler.cpp multiviewBatchRenderer.cpp
```

**结论：** `writeImageFile` 只保留在旧 handler；默认 `outputMode` 是 `memory`；memory 路径使用 `MultiviewBatchRenderer`。

- [x] 静态检查工程文件：

```bash
rg -n "<Optimization>|memoryFrameSink|multiviewBatchRenderer|multiviewRenderPlan" multiview.vcxproj multiview.vcxproj.filters
```

**结论：** 新文件已加入工程；`Release|Win32` 和 `Release|x64` 为 `MaxSpeed`，Debug 配置仍为 `Disabled`。

## 任务 9：Windows/OSG 真实验收

**状态：** 当前 Mac 工作区无法完成。缺少 Visual Studio、OSG、base、真实模型路径和 Windows 图形环境。

**在 Windows 依赖环境中构建：**

```cmd
msbuild multiview.vcxproj /p:Configuration=Release /p:Platform=x64
```

**运行目标性能用例：**

```cmd
target\testmulitview.exe -file <model.obj> -type obj -outdir <unused-for-memory-mode> -angle 90 -pre 3 -resolution 150 -output memory
```

**验收输出必须包含：**

```text
multiview_memory_frames=72900
multiview_memory_frames_captured=72900
multiview_graphics_pbuffer=1
multiview_graphics_double_buffer=0
multiview_graphics_vsync=0
multiview_graphics_window_decoration=0
multiview_memory_bytes=4920750000
multiview_readback_errors=0
multiview_process_seconds=<value less than 10.0>
```

**当前新增性能证据：**

用户在上一版 memory 路径上实测：

```text
render_call_seconds=1215.48
render_seconds=927.688
readback_seconds=287.388
process_seconds=1218.39
```

`1215.48 / 72900 = 0.01667s`，等于 60Hz 一帧时间，判断根因是 memory 路径仍走窗口双缓冲/帧交换节流。本轮已改成 memory 模式离屏 pbuffer + 单缓冲 + 前缓冲读写。下一次 Windows/OSG 运行时优先确认 `multiview_graphics_*` 四个字段。

**如果超过 10 秒：**

不要盲改。根据计时字段判断瓶颈：

- `renderSeconds` 高：继续减少 OSG 事件、状态切换、场景遍历和 viewer 额外 handler。
- `readbackSeconds` 高：改为 PBO 异步 readback 或多缓冲 readback。
- `process_seconds` 明显高于 `totalSeconds`：优化模型加载、viewer setup、4.92GB buffer 分配和渲染以外的固定开销。

## 代码审查结论

已做代码审查并处理当前能在源码层面修复的问题：

- `GL_PACK_ALIGNMENT` 未设置导致 150x150 RGB readback 可能写越界。
- 未统计实际 post-draw callback 成功捕获帧数，可能形成假验收。
- 未保存/重置 `GL_PACK_ROW_LENGTH`、`GL_PACK_SKIP_ROWS`、`GL_PACK_SKIP_PIXELS`，可能受上游 OpenGL pack state 影响。
- `bytesCaptured` 原先没有 readback 错误前提。
- 裸 `glReadPixels` 在 `viewer->frame()` 后调用存在上下文风险，已改到 camera post-draw callback。
- 端到端计时原先不包含 buffer 分配，已增加 `multiview_end_to_end_seconds`。
- `angle * pre` 原先在构造初始化列表中先做 `int` 乘法，异常大参数可能在校验前溢出；已改为显式 `uint64_t` 乘法检查后再赋值。
- 模型加载失败或空目录原先仍可能进入渲染；已改为返回错误。
- 图形上下文创建失败原先可能空指针崩溃；已改为返回错误。
- `MemoryFrameSink` 分配失败现在会通过 memory 路径输出 `multiview_error=...`。
- 旋转中心已缓存，减少 72,900 帧循环中的 OSG bound 查询。

仍属于后续业务接口范围、当前未实现：

- `MemoryFrameSink` 当前是 `main()` 内局部对象，足以做性能统计和“不落盘”验证；如果下游模块要继续消费这 72,900 张 raw RGB，需要补一个明确的内存交接 API。

仍需在目标环境补证据：

- Windows `Release|x64` 构建成功。
- 真实模型运行输出 `frames=72900`、`frames_captured=72900`、`readback_errors=0`。
- 运行前后 `outdir` 不增加 `72,900` 个文件。
- 抽样帧与旧 JPEG 路径在视角顺序、上下方向和可接受视觉一致性上通过。
- `multiview_process_seconds < 10.0`。

## 当前限制

当前目录不是 git 仓库，无法提交 spec/plan commit，也无法用 git diff 生成标准审查范围。

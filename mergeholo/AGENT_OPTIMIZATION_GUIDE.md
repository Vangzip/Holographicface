# MergeHolo Agent Optimization Guide

这份文档给后续接手的 agent 使用。目标是快速理解当前 MergeHolo 的结构、已经验证过的优化、失败路径和测试方法，避免重复走弯路。

## 当前稳定目标

MergeHolo 不是重新发明一套 Holo 算法，而是把相机采集、Holo pipeline、UI、后续打印/消费入口接成一个可维护的 Qt 项目。

当前默认流程：

```text
CaptureWindow
-> CaptureSession / LightFieldCapture
-> output/input/0.jpg + 0.tiff
-> HoloPipeline
-> DepthStage
-> MeshStage
-> MultiviewStage
-> ElementalStage
-> ElementalMemoryResult
```

当前性能目标是把一次确认后的完整流程压到十秒量级。最新稳定测试约 16.8 秒，后续优化应继续压缩 mesh 和 multiview。

## 必须优先遵守

1. 只改 `mergeholo`，旧 `Holo` 和 `holocamera` 默认只读参考。
2. 只用 Release 做性能判断。Debug 对 PCL/OSG/Qt 性能没有参考意义。
3. 不提交运行输出和替换包目录：
   - `00-bin/`
   - `FF-tmp/`
   - `output/`
   - `runs/`
   - `pipeline_*/`
4. 每次优化后用同一份输入跑 `--stage all`，比较日志里的 stage timing。
5. 不要为了“看起来省事”把 PCL 对象裸指针泄漏掉。PCL 析构崩溃已经通过配置和释放顺序解决。

## 项目结构速览

```text
apps/mergeholo_main.cpp
  CLI/UI 入口分发。

widgets/CaptureWindow.*
ui/CaptureWindow.ui
  Qt 主窗口。负责实时预览、拍照、确认、重新拍照、进度展示。

camera/CaptureSession.*
camera/LightFieldCapture.*
  光场相机采集。LightFieldCapture 负责 SDK 初始化和帧读取。

pipeline/HoloPipeline.*
  pipeline 总调度。决定哪些 stage 运行、内存结果如何传递、日志如何写出。

pipeline/stages/
  DepthStage: RGB + TIFF -> 点云内存/PLY
  MeshStage: 点云 -> PCL mesh
  ModelStage: mesh -> OBJ/贴图。当前 all 流程通常跳过。
  MultiviewStage: memory mesh 或 OBJ -> multiview memory
  ElementalStage: multiview memory -> elemental memory

pipeline/multiview/
  MultiviewMemoryResult: multiview 内存结果和统计字段。
  PclMeshOsgBuilder: PCL mesh 直接转 OSG Geometry。

pipeline/elemental/
  ElementalProcessor: 当前默认 elemental 内存转换。
  ElementalAtlasDirectSink: 0710 direct atlas scatter 实验接口，保留但默认不启用。

vendor/multiview/
  OSG atlas 渲染、MemoryFrameSink、MemoryAtlasPageSink 等基础渲染内存接口。
```

## 9030 打印诊断与日志

当前打印模块的文件日志仅有启动诊断：

```text
runs/latest/imc60g_startup.log
```

它由 `apps/mergeholo_main.cpp` 写入，包含 IMC60G 配置、轴映射和启动诊断信息。打印运行中的连接/回零状态、逐行进度、取消结果和错误由 `PrintController` 的 `statusChanged`、`progressChanged`、`errorChanged` 信号发送到 `Print9030Dialog`，当前不落盘。

排查实际打印问题时，先保存对话框错误栏的完整文本，再查看启动日志。不要将 `runs/imc60g-acceptance/*/print_flow.log` 视为自动生成的现场运行日志，它们仅来自历史验收流程。

## 已验证成功的优化

### 1. PCL 析构崩溃修复

现状：

```text
Debug: /MDd
Release: /MD
Debug/Release: /arch:AVX
AdditionalOptions: /FS /utf-8
```

代码原则：

```text
NormalEstimation 用 std::unique_ptr，直接 reset。
不要在析构前调用 ne->setInputCloud(nullptr)。

GreedyProjectionTriangulation 用 std::unique_ptr。
gp3 析构前先清空 input cloud 和 search tree，再 reset。
```

这个问题不是 `Ptr` 本身的问题，也不应该用裸指针泄漏规避。

### 2. mesh 不再多子进程

现在只处理一个 PLY，不需要为了规避析构崩溃启动多个子进程。若未来重新引入批处理，优先做进程级任务队列，而不是把单次 UI 流程复杂化。

### 3. multiview 内存结果

新版 multiview 去掉显示，把 atlas 渲染结果放进 `MemoryFrameSink`。`ElementalStage` 直接读这块内存，不再去文件夹读取多视角 JPG。

成功路径日志特征：

```text
[multiview] output mode: atlas-memory
[elemental] using multiview memory buffer directly; no file load or duplicate view cache.
```

### 4. elemental 线程化内存处理

`elemental_writer_threads=0` 自动使用硬件线程。当前配置实测通常显示：

```text
[elemental] writer threads: 12
[elemental] stored output images in memory in ~2.8-3.0s
```

如果后续结果要给打印或显示继续消费，不要退回“写文件再读文件”的虚拟产物方式。保持 `ElementalMemoryResult` 在内存中传递。

### 5. pipeline0710 memory mesh 路径

已合入 `pipeline/`。不要再保留 `pipeline_0710` 目录。

原路径：

```text
MeshStage
-> ModelStage 写 OBJ/贴图
-> MultiviewStage 用 osgDB 读 OBJ
```

当前路径：

```text
MeshStage 生成 MeshMemoryResult
-> HoloPipeline 跳过 ModelStage
-> PclMeshOsgBuilder 把 PCL mesh 直接构成 OSG Geometry
-> MultiviewStage 渲染
```

成功路径日志特征：

```text
[model] skipped: multiview will consume memory mesh directly.
[multiview] model source: memory mesh
```

这条优化在一次测试中将总耗时从约 22.5 秒降到约 16.8 秒。

## 已验证失败或暂不采用

### direct-atlas-elemental 默认关闭

`ElementalAtlasDirectSink` 能在 atlas page readback 后直接 scatter 到 elemental 输出内存，理论上可以跳过完整 multiview memory buffer。

但当前实现对输出内存是大跨度散写，缓存局部性差。实测：

```text
direct elemental scatter: 20.740s
total measured: 33.780s
```

因此默认关闭：

```cpp
const bool directAtlasElemental = false;
```

如果后续要重启它，必须先改变内存布局或写入策略，并用同一输入做 A/B 测试。

## 标准测试命令

Release 构建：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\mergeholo\scripts\build.ps1 -Config release
```

完整流程测试：

```powershell
.\mergeholo\00-bin\mergeholo.exe --config .\mergeholo\output\holo_config.ui.ini --stage all
```

如果 `output/holo_config.ui.ini` 不存在，需要先通过 UI 拍照确认生成，或从 `config/ui_pipeline_template.ini` 准备一份同等配置。

重点看：

```text
[timing] stage depth
[timing] stage mesh
[timing] stage model
[timing] stage multiview
[timing] stage elemental
[timing] total measured
```

## 常见坑

### 相机配置改了但运行仍读旧路径

UI 可能读取的是运行时复制到 `output/holo_config.ui.ini` 的配置，而不是你刚改的 `00-bin/config/default_camera.ini`。先看程序启动日志里的：

```text
config camera path: ...
```

再反查该路径来自哪个 ini。

### 084C 配置初始化失败

错误：

```text
pixel size : -1e+10 is error value.
JpIParse Init failed
```

处理顺序：

1. 对比 `holocamera` 成功运行版本的 SDK DLL。
2. 对比 `00-bin/config/084C` 目录内容。
3. 确认 `JpLFDll-v4.1.1` include/lib/windows 路径一致。
4. 不要只改 ini 文件名，要确认实际复制到 `00-bin/config` 的目录存在且完整。

### VS 版本混用导致链接错误

如果出现 `_Thrd_sleep_for`、`__std_find_*` 一类链接错误，通常是 MSVC 工具链或对象文件混用。处理：

```text
固定 VS2019 v142 x64
删除 .qmake.stash
Run qmake
Clean/Rebuild Release
不要复用其他 MSVC 版本生成的 obj
```

### README 或中文日志乱码

文档应保存为 UTF-8。PowerShell 控制台显示乱码不一定代表文件编码错，但如果 README 本身已乱码，直接重写为 UTF-8。

## 下一步优化建议

1. mesh 阶段
   - 分析 NormalEstimation 和 GreedyProjectionTriangulation 的参数。
   - 尝试缓存搜索结构，减少重复构建。
   - 评估是否可以降低点云数量但保持视觉质量。

2. multiview 阶段
   - 继续优化 atlas size 和 page count。
   - 减少 OSG viewer/context 初始化成本。
   - 研究 PBO 异步 readback，避免 `glReadPixels` 同步阻塞。

3. elemental 阶段
   - 保持内存结果，不落盘。
   - 如果尝试 direct scatter，先改成 cache-friendly layout 或 tiled gather。
   - 所有优化必须保持后续打印/消费可读取 `ElementalMemoryResult`。

4. UI 流程
   - 确认处理期间按钮禁用。
   - 确认完成后结果留在 `mergeholo/output`，内存结果留给打印逻辑。
   - 避免每次确认都重建不必要的渲染环境。

## 提交建议

每次只提交一个清晰主题：

```text
性能优化提交：代码 + 测试日志摘要
文档提交：README/agent guide
清理提交：.gitignore 或删除已跟踪废弃文件
```

当前 `pipeline_0710`、`pipeline_0709`、`pipeline_0708` 这类目录是历史替换包。验证成功合入后应删除本地目录，不要提交。

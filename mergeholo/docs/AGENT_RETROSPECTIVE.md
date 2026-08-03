# MergeHolo Agent Retrospective

用途：给后续智能体快速接手 `mergeholo` 使用。本文不是完整项目说明，而是接手时的判断框架、历史决策、验证方式和改进路径。

原始对话归档：

```text
C:/Users/jumper/.codex/archived_sessions/rollout-2026-07-06T10-19-46-019f3539-65b1-7e80-8168-e05679cb8a21.jsonl
```

详细人类复盘：

```text
mergeholo/docs/ENGINEERING_REVIEW_2026-07-06_2026-07-09.md
```

项目结构与流程说明：

```text
mergeholo/docs/PROJECT_STRUCTURE_AND_FLOW.md
mergeholo/README.md
```

## 接手时先读什么

按这个顺序读，不要一上来扫全仓库：

1. `mergeholo/README.md`
2. `mergeholo/docs/PROJECT_STRUCTURE_AND_FLOW.md`
3. `mergeholo/docs/ENGINEERING_REVIEW_2026-07-06_2026-07-09.md`
4. `mergeholo/apps/mergeholo_main.cpp`
5. `mergeholo/widgets/CaptureWindow.cpp`
6. `mergeholo/pipeline/HoloPipeline.cpp`
7. `mergeholo/pipeline/PipelineModule.pri`
8. 当前要改的 stage，例如 `pipeline/stages/MultiviewStage.cpp` 或 `pipeline/elemental/ElementalMemoryTransform.cpp`

核心心智模型：

```text
UI/CLI entry
-> camera capture or existing input
-> pipeline config
-> depth
-> mesh
-> model
-> multiview memory
-> elemental memory
-> pipeline.log
```

## 项目边界

`Holo` 和 `holocamera` 是旧项目来源和对照组。默认不要改它们，除非用户明确要求。

`mergeholo` 是当前融合项目。新代码、配置、构建脚本、运行输出和文档优先放这里。

`mergeholo/output`、`mergeholo/runs`、`mergeholo/00-bin` 多数是运行产物。不要把大输出文件当作源码提交。

如果要替换 pipeline 版本，不要直接在 `.pro` 里到处加文件。优先保持 `mergeholo/pipeline` 作为稳定入口，通过 `pipeline/PipelineModule.pri` 集中维护模块文件列表。

## 历史关键决策

### 1. 新项目隔离旧项目

用户要求老项目只读，所有融合和重构都在 `mergeholo` 中完成。这个边界必须继续遵守。

### 2. Release 和 MSVC2019 是默认工作模式

用户明确说“不要 debug 用 release”和“我要用 2019”。

推荐环境：

```text
Qt 5.15.0 MSVC2019 64-bit
MSVC v142 / x64
Release
OpenCV 4.5.0
PCL 1.12.1-rc1
OSG 3.6.5
OE32
```

`Pri/common.pri` 已经限制 `QMAKE_MSC_VER` 在 VS2019 范围。遇到 `_Thrd_sleep_for`、`_Cnd_timedwait_for_unchecked`、`__std_find_last_trivial_1` 这类 LNK 错误，优先怀疑 MSVC toolset 或旧 `.obj` 混用，不要先改业务代码。

### 3. PCL 析构问题不能靠泄漏解决

历史上 PCL mesh 析构曾崩溃。最终方向是：

- 统一 `/MD`、`/MDd`。
- 使用 `/arch:AVX`。
- 用 RAII 管资源。
- `gp3.reset()` 前断开 input cloud 和 search method。
- 不再用多子进程绕过析构。

所以不要恢复裸指针泄漏或 mesh worker pool。

### 4. UI 直接进程内跑 pipeline

UI 确认后不再每次启动子进程。`CaptureWindow` 在线程中直接调用 `runHoloPipelineCli`，减少重复初始化渲染环境。

UI 状态由 `Starting`、`Live`、`Frozen`、`Processing`、`Done`、`Error` 管理。改 UI 时优先维护这个状态机，不要只改按钮 enable/disable。

### 5. 性能优化以日志为准

性能判断以 `pipeline.log` 和测试程序为准。

已知一组优化前后数字：

```text
早期 multiview + elemental:
multiview:  9.814s
elemental: 22.679s
total:     32.493s

近期完整 pipeline:
wall:      16.904s
depth:      0.029s
mesh:       4.247s
model:      2.530s
multiview:  7.537s
elemental:  2.556s
```

不要只凭主观感受判断变快或变慢。改动后必须跑同一类输入并比较日志。

## 当前重要模块

### 入口

```text
mergeholo/apps/mergeholo_main.cpp
```

负责：

- 默认启动 UI。
- `--capture`
- `--import-capture`
- `--capture-and-run`
- `--pipeline`
- 默认相机配置路径解析。

如果用户说配置不生效，先看这里的读取顺序。

### UI

```text
mergeholo/ui/CaptureWindow.ui
mergeholo/widgets/CaptureWindow.h
mergeholo/widgets/CaptureWindow.cpp
```

负责：

- 实时 RGB/深度预览。
- 拍照定格。
- 确认后写 `output/input/0.jpg` 和 `output/input/0.tiff`。
- 写 `output/holo_config.ui.ini`。
- 进程内运行 pipeline。
- 在 `pipeline.log` 追加 UI 总耗时。

### 相机

```text
mergeholo/camera/LightFieldCapture.*
mergeholo/camera/CaptureSession.*
mergeholo/config/default_camera.ini
mergeholo/scripts/build.ps1
```

当前 084C 依赖 v4 光场相机 runtime。历史问题是 `default_camera.ini` 看似改了但实际仍走旧 `holoConf-023C`，后来修成从 ini 读取并 fallback 到 `config/084C`。

如果出现：

```text
pixel size : -1e+10 is error value.
JpIParse Init failed
```

先对照可运行的 `holocamera` runtime、DLL、config 目录和 camera type/serial，不要只改 ini。

### Pipeline 编排

```text
mergeholo/pipeline/HoloPipeline.cpp
mergeholo/pipeline/PipelineConfig.*
mergeholo/pipeline/PipelineTiming.*
mergeholo/pipeline/PipelineLogger.*
mergeholo/pipeline/PipelineContext.h
mergeholo/pipeline/DepthMeshModelMemory.h
```

负责：

- 解析 config。
- 判断跑哪些 stage。
- 记录 stage timing。
- 在 depth/mesh/model 可行时走内存链路。
- 在阶段后及时清理内存。
- 输出 `pipeline.log`。

### Stage

```text
mergeholo/pipeline/stages/DepthStage.*
mergeholo/pipeline/stages/MeshStage.*
mergeholo/pipeline/stages/ModelStage.*
mergeholo/pipeline/stages/MultiviewStage.*
mergeholo/pipeline/stages/ElementalStage.*
```

每个 stage 只负责自己的阶段。不要把 UI、配置查找、日志格式、相机逻辑塞回 stage。

### Multiview

```text
mergeholo/pipeline/stages/MultiviewStage.cpp
mergeholo/vendor/multiview/*
mergeholo/pipeline/multiview/*
```

关键点：

- OSG pbuffer 离屏渲染。
- atlas 自动选择，查询 `GL_MAX_TEXTURE_SIZE`。
- `MemoryFrameSink` 保存所有 view。
- 输出 mode 应优先是 `atlas-memory`。
- 重点关注 render、readback、copy 分项耗时。

### Elemental

```text
mergeholo/pipeline/elemental/ElementalProcessor.cpp
mergeholo/pipeline/elemental/ElementalMemoryTransform.cpp
mergeholo/pipeline/elemental/ElementalMemoryResult.h
mergeholo/pipeline/tests/test_elemental_memory_transform.cpp
```

关键点：

- 当前目标是 memory only，`files_written=0`。
- 结果需要能被后续程序消费，所以不能只做“虚拟可读产物”。
- 默认 `elemental_writer_threads=0` 表示自动线程。
- 优化重点是访存模式和缓存局部性，不是盲目加线程。

## 标准验证命令

所有命令默认从仓库根目录或 `mergeholo` 目录执行。优先 Release。

### 构建

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\mergeholo\scripts\build.ps1 -Config release
```

如果切过 Qt Kit 或 MSVC 版本：

```powershell
Remove-Item .\mergeholo\.qmake.stash -Force -ErrorAction SilentlyContinue
powershell -NoProfile -ExecutionPolicy Bypass -File .\mergeholo\scripts\build.ps1 -Config release -Clean
```

### 启动冒烟

```powershell
.\mergeholo\00-bin\mergeholo.exe --mergeholo-help
```

### Pipeline dry-run

```powershell
.\mergeholo\00-bin\mergeholo.exe --pipeline --config .\mergeholo\config\holo_config.microtest.ini --stage all --dry-run
```

### UI 输出配置后的真实 pipeline

```powershell
.\mergeholo\00-bin\mergeholo.exe --config .\mergeholo\output\holo_config.ui.ini --stage all
Get-Content .\mergeholo\output\pipeline.log
```

### 真机采集一帧

```powershell
.\mergeholo\00-bin\mergeholo.exe --capture --max-frames 1 --duration 15 --no-preview --save-dir .\mergeholo\output\capture_test
```

### Elemental 纯逻辑测试

```powershell
cmd.exe /s /c 'call "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && cd /d C:\wzp\Holographicface\mergeholo\00-bin\elemental_tests && cl /nologo /O2 /MD /EHsc /std:c++17 /utf-8 /DNOMINMAX /DWIN64 ..\..\pipeline\tests\test_elemental_memory_transform.cpp ..\..\pipeline\elemental\ElementalMemoryTransform.cpp /Fe:test_elemental_current.exe'
.\mergeholo\00-bin\elemental_tests\test_elemental_current.exe
```

测试应覆盖：

- 尺寸计算。
- pixel mapping。
- source Y flip。
- view row flip。
- OpenGL bottom-up source rows。
- source 小于 target 时补零。
- 单线程和多线程一致。
- benchmark checksum。

## 处理问题的推荐流程

### 构建失败

1. 确认 Release。
2. 确认 Qt Kit 是 MSVC2019 64-bit。
3. 删除 `.qmake.stash`，重新 qmake。
4. 清理 `FF-tmp` 或至少确认中间目录包含正确 `msvc192x`。
5. 看 `Pri/common.pri`、`Pri/holo_pipeline.pri`、`Pri/cuda.pri`。
6. 链接错误先查 ABI/toolset，不要直接改算法代码。

### 相机失败

1. 读实际输出的 `config camera path`。
2. 看 `default_camera.ini` 是否被入口解析。
3. 看 `00-bin/config` 是否有运行时副本覆盖。
4. 对比 `holocamera` 可运行目录的 DLL、config、camera type、serial。
5. 084C 优先确认 v4 runtime 是否部署。
6. 用 `--capture --max-frames 1 --duration 15 --no-preview` 建立最小闭环。

### UI 卡住或状态错

1. 看 `CaptureWindow::setState`。
2. 看 `startCamera`、`pollCameraFrame`、`captureFrame`、`startProcessing`、`finishPipelineRun`。
3. 确认处理线程退出后是否回到 UI 线程更新状态。
4. 确认 `pipelineThread_` 生命周期没有悬挂。
5. 不要在 UI 线程直接跑重 pipeline。

### Pipeline 结果错

1. 用 `--dry-run` 检查路径和 stage。
2. 单独跑出错 stage。
3. 检查 `holo_config.ui.ini` 或 `default_pipeline.ini`。
4. 看 `pipeline.log` 中 stage result。
5. 对比旧项目或候选 pipeline 目录。

### 速度变慢

1. 先拿同一输入重复跑。
2. 比较 `pipeline.log`。
3. 把问题归属到 stage。
4. multiview 看 render/readback/copy。
5. elemental 看 writer threads、memory mode、checksum、benchmark。
6. depth/mesh/model 看是否退回文件链路。
7. 只改一个变量，跑一次，记录结果。

## 后续改进方向

优先级建议：

1. 继续压缩 `multiview`，它在最新日志里约 7.537s，是下一大头。
2. 分析 mesh/model 约 6.777s 的文件写入和可缓存部分。
3. 给 UI 进度增加 stage 级实时回传，而不是只写最终日志。
4. 给相机配置做 profile 管理，例如 `023C`、`084C`、`182C`。
5. 建立正式测试脚本，把 elemental 编译和 benchmark 固化到 `scripts`。
6. 如果后续消费者支持流式处理，评估 elemental 分块产出，降低一次 materialize 4.58 GiB 的峰值成本。
7. 整理 `pipeline_0708`、`elemental_0709`、`elemental_optimized` 候选目录，保留说明，避免未来智能体误以为它们都已接入主线。

## 不要做的事

- 不要默认修改 `Holo`、`holocamera`。
- 不要恢复 PCL 裸指针泄漏。
- 不要恢复 mesh 多子进程规避方案，除非用户明确要求批处理多 PLY。
- 不要用 Debug 结果判断性能。
- 不要混用 VS2022/VS2026 的 `.obj` 和 VS2019 库。
- 不要把 `mergeholo/output` 大量产物加入源码。
- 不要仅凭“感觉更快”合并优化。
- 不要让 stage 直接依赖 UI。

## 建议后续智能体使用的 skills

- `using-superpowers`：开始任务时先检查技能。
- `diagnosing-bugs` 或 `systematic-debugging`：处理崩溃、相机失败、链接错误、结果错误。
- `verification-before-completion`：声称修好前必须跑验证命令。
- `codebase-design`：继续解耦、拆模块、设计稳定接口时使用。
- `tdd` 或 `test-driven-development`：为 elemental、配置解析、pipeline stage 增加测试时使用。
- `receiving-code-review`：用户给出修改意见或指出实现方向不对时使用。
- `requesting-code-review`：大改完成后做检查。

## 给下一个智能体的工作方式

先读日志，再改代码。先跑最小闭环，再扩大场景。先判断问题属于构建、配置、相机、UI、stage、性能哪一类，再进入对应模块。

对这个项目最有用的不是一次性大改，而是小步：

```text
读现状
-> 形成假设
-> 找一个能验证的命令
-> 小改
-> 构建
-> 跑同一输入
-> 读日志
-> 记录结论
-> 再继续
```

如果优化不成立，立即回退。这个项目已经证明，能回退、能对比、能复测，比硬推进更重要。

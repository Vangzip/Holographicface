# MergeHolo 对话工程复盘

复盘对象：`C:/Users/jumper/.codex/archived_sessions/rollout-2026-07-06T10-19-46-019f3539-65b1-7e80-8168-e05679cb8a21.jsonl`

时间范围：2026-07-06 到 2026-07-09

项目目标：把旧 `Holo` 后处理管线和 `holocamera` 光场相机采集程序融合为新的 Qt/C++ 项目 `mergeholo`，并持续优化结构、稳定性、性能和可维护性。

## 一句话总结

这次对话的核心价值不是单次修 bug，而是完成了一次典型的工程化演进：先把两个历史项目安全融合成一个可运行项目，再用编译、dry-run、真实相机、UI 流程、阶段计时和微基准建立反馈闭环，随后按瓶颈逐步重构、内存化、优化和回退验证，最终形成了一个模块化、可配置、可测试、可继续优化的 MergeHolo 工程。

## 最终状态摘要

当前主线提交已经形成清晰演进：

```text
e0fb0c1 Add merged MergeHolo project
b85493d Document MergeHolo usage and flow
9385f95 Implement merged point cloud pipeline
c9c6bf6 Fix PCL mesh teardown ownership
7ef6fc6 Limit mesh stage to one PLY input
0a107f6 Add MergeHolo camera capture UI
59df43b Add multiview0707 pbuffer renderer
a8ade5d Use in-memory multiview frames for elemental loop
13aaa33 Keep elemental output in memory and write pipeline log
cb902f3 Decouple mergeholo pipeline modules
336eb28 Restructure mergeholo modules
ed951c9 Integrate pipeline 0708 memory transform
0babeab Revert "Integrate pipeline 0708 memory transform"
4d22fa7 Reapply "Integrate pipeline 0708 memory transform"
6a61131 Use automatic elemental worker threads by default
913db68 Auto-select multiview atlas size
42a8dae Run UI pipeline in process
d21e1a8 Keep depth mesh model stages in memory
06434c4 Materialize elemental memory output
e645ef0 Use VS2019 toolchain for mergeholo
261dcf7 Use optimized elemental memory transform
afda352 Use v4 light-field camera runtime for 084C
```

最新 `mergeholo/output/pipeline.log` 的关键结果：

```text
result_code=0
wall_seconds=16.904

depth.seconds=0.029
mesh.seconds=4.247
model.seconds=2.530
multiview.seconds=7.537
elemental.seconds=2.556

depth_to_mesh=memory
mesh_to_model=memory
multiview.mode=atlas-memory
elemental.mode=memory
elemental.files_written=0
```

一次早期对比基线中，`multiview + elemental` 为：

```text
multiview:  9.814s
elemental: 22.679s
合计:      32.493s
writer threads: 12
```

这说明优化不是凭感觉推进，而是靠日志和阶段计时证明效果。

## 需求演进时间线

1. 新建 `mergeholo`，要求旧 `Holo` 和 `holocamera` 只读，所有新改动放到 `mergeholo`。
2. 明确 Qt 编译器版本：Qt 5.15 MSVC2019 64-bit，MSVC v142 x64，不使用 MinGW 或 32 位 Kit。
3. 先写 README 和项目流程文档，保证用途、用法、数据流可解释。
4. 处理 PCL mesh 析构崩溃，从临时泄漏裸指针改为 RAII 和析构前断开内部引用。
5. mesh 阶段改成只处理单个 PLY，去掉多子进程 `-mesh` 规避方案。
6. 增加相机 UI：实时 RGB/深度预览、拍照定格、确认处理、重新拍照、结果输出到 `mergeholo/output`。
7. 接入 `multiview0707_2`，把多视角结果放到内存，不再靠文件夹里的 JPG 给 elemental 读取。
8. 改造 elemental：结果也保存在内存，并输出日志和阶段耗时。
9. 按低耦合高聚合目标重构目录结构，拆分 `apps`、`widgets`、`camera`、`pipeline`、`pipeline/stages`、`config`、`ui`。
10. 接入、回退、再接入 `pipeline_0708`，保留可维护替换接口 `PipelineModule.pri`。
11. 针对速度变慢的问题读日志，从算法和优化角度定位瓶颈。
12. 按优化路线逐步推进：UI 进程内执行、depth/mesh/model 内存链路、multiview atlas 自动选择、elemental 自动线程、elemental materialize。
13. 解决 Qt 构建链接错误，锁定 MSVC2019 工具链，避免不同 MSVC 版本对象文件和运行库混用。
14. 对比 `elemental_optimized`、`elemental_0709` 与 baseline，使用单元测试和 benchmark 判断是否有正提升。
15. 修复 `default_camera.ini` 不生效和 084C 相机解析失败问题，改用 v4 光场相机 SDK/runtime。

## 正确思路

### 1. 先确定边界，再写代码

一开始没有直接在旧项目里乱改，而是明确：

- `Holo` 和 `holocamera` 是只读来源。
- 新项目、配置、构建脚本、运行输出都归入 `mergeholo`。
- 旧项目保留为对照组，出问题时可以和可运行版本比较。

这个边界很重要。它避免了历史项目继续变乱，也让新项目可以按新的目录结构重建。

### 2. 先能跑，再重构，再优化

这次顺序是正确的：

```text
可编译
-> 可启动
-> dry-run 通过
-> 少量数据流程通过
-> UI 可操作
-> 真机采集通过
-> 阶段计时可读
-> 找瓶颈
-> 局部优化
-> 再测试
```

如果一开始就追求最终架构和 10 秒目标，会同时面对编译、运行、数据格式、相机 SDK、PCL、OSG、OpenGL、UI 状态和性能瓶颈，问题空间太大。

### 3. 每个问题都要有可重复的反馈闭环

这次反复使用了这些反馈闭环：

- `--mergeholo-help`：验证 exe 能启动、依赖能加载。
- `--pipeline --config ... --dry-run`：验证配置解析和阶段分发。
- Release 构建：验证 qmake/nmake/链接环境。
- `test_elemental_memory_transform.cpp`：验证 elemental 内存变换的纯逻辑。
- `pipeline.log`：验证真实流程阶段耗时和内存模式。
- 真机 `--capture --max-frames 1`：验证 084C 相机 runtime 和配置。
- `git diff --no-index`：对比 `pipeline`、`pipeline_0708`、`elemental_optimized`、`elemental_0709`。

可靠的工程判断来自这些可重复信号，而不是只看代码猜。

### 4. 性能优化先测量，后假设，再改动

速度问题不是直接上多线程，而是先读日志：

- 发现 `elemental` 曾经占 22.679 秒，是最大热点。
- 发现 `multiview` 也有 GPU 渲染、readback、copy 的分项耗时。
- 发现 UI 每次起子进程会重复创建渲染环境。
- 发现磁盘 JPG 中转和重复加载会吞掉大量时间。

因此优化方向变成：

- UI 进程内调用 pipeline，避免子进程初始化。
- multiview 用 pbuffer + atlas 一次性 GPU 渲染/读回。
- multiview 输出放内存，elemental 直接消费内存。
- depth/mesh/model 尽量走内存结果，并在阶段后及时 clear。
- elemental 调整访存顺序和线程策略，而不是盲目增加线程数。

## 问题发现与解决路径

### 问题 1：PCL mesh 析构崩溃

错误思路：把 `pcl::NormalEstimation` 和 `pcl::GreedyProjectionTriangulation` 用裸指针 `new` 出来，并故意不 `delete`，靠泄漏绕过崩溃。

正确思路：

- 先确认不是 `Ptr` 本身的问题。
- 统一 Debug/Release 运行库：Debug `/MDd`，Release `/MD`。
- 启用 `/arch:AVX`，匹配 PCL 构建约束。
- 用 `std::unique_ptr` 恢复正常生命周期。
- 在 `gp3.reset()` 前先断开 `setInputCloud` 和 `setSearchMethod`。
- 不对 `NormalEstimation` 调 `setInputCloud(nullptr)`，避免触发已定位的崩溃点。

工程收获：不要把资源泄漏当修复。崩溃类问题要定位对象所有权、析构顺序、运行库 ABI 和第三方库内部引用。

### 问题 2：mesh 多子进程规避方案变成负担

背景：早期为了绕过 PCL 析构崩溃，用多子进程执行 mesh。

新事实：PCL 析构问题已经修复，用户当前只需要一个 PLY。

解决：

- 删除 worker pool、`CreateProcessW`、worker 日志等复杂逻辑。
- `mesh` 阶段只处理一个明确输入。
- 多个 PLY 时要求传 `--input`，防止误处理。

工程收获：历史 workaround 一旦失去必要性，就要删掉。否则它会成为性能、维护和排错成本。

### 问题 3：项目结构耦合严重

用户指出不同功能混在同一个文件里，希望标准 `.h/.cpp` 拆分、默认配置抽离、UI 使用 `.ui` 文件。

最终结构方向：

```text
mergeholo/
  apps/                  程序入口
  ui/                    Qt Designer .ui
  widgets/               UI 逻辑
  camera/                相机 SDK 封装和采集会话
  config/                默认相机和管线配置
  pipeline/              管线协调
  pipeline/stages/       depth、mesh、model、multiview、elemental 阶段
  pipeline/elemental/    elemental 内存变换和结果
  pipeline/multiview/    multiview 配置和内存结果
  vendor/                从旧项目迁入的第三方/历史业务源码
  Pri/                   qmake 依赖和工具链配置
```

关键设计：

- `apps/mergeholo_main.cpp` 只做命令分发。
- `widgets/CaptureWindow.*` 管 UI 状态和用户交互。
- `camera/CaptureSession.*` 管相机采集和保存。
- `pipeline/HoloPipeline.cpp` 管阶段编排。
- `pipeline/stages/*.cpp` 分别承接每个阶段。
- `pipeline/PipelineConfig.*` 统一解析配置。
- `pipeline/PipelineLogger.*` 统一输出日志。
- `pipeline/PipelineModule.pri` 作为后续替换 pipeline 版本的工程接入点。

工程收获：低耦合不是把文件拆多，而是让每个文件只承担一种变化原因。入口、UI、相机、配置、阶段、日志、性能统计都应该独立变化。

### 问题 4：UI 流程既要实时预览，又要稳定处理

用户要求：

- 实时显示 RGB 和深度图。
- 点击拍照后定格当前帧，拍照按钮不可再按。
- 确认和重新拍照只在拍照后启用。
- 确认后把 RGB/深度送入 pipeline，显示处理进度。
- 处理过程中所有按钮不可按。
- 最终结果保存到 `mergeholo/output`。

设计方案：

- 用 `CaptureWindow.ui` 创建可手动维护的界面。
- 用状态机表达 UI：`Starting`、`Live`、`Frozen`、`Processing`、`Done`、`Error`。
- 实时帧通过 `QTimer` 轮询相机，显示到 `QLabel`。
- 深度图用 OpenCV 归一化和 colormap 生成预览。
- 点击拍照时 clone 当前 `cv::Mat` 到内存，停止实时更新。
- 点击确认时保存 `0.jpg` 和 `0.tiff` 到 `mergeholo/output/input`。
- 写出 `holo_config.ui.ini`，再在线程内调用 `runHoloPipelineCli`。
- 处理结束追加 `[ui] confirm_to_finish_seconds` 到日志。

工程收获：UI 的复杂度要用状态机管理，不要靠按钮之间互相猜状态。

### 问题 5：pipeline 版本替换容易造成散乱改动

用户后续可能反复替换 `pipeline_0708`、`pipeline_0709` 等版本。

解决方案：

- 保持 `mergeholo/pipeline` 是工程稳定入口。
- 外部候选版本作为对比目录，例如 `pipeline_0708`、`pipeline/elemental_0709`。
- 用 `PipelineModule.pri` 集中列出 pipeline 模块源码和头文件。
- 替换时只需要把候选实现合并到稳定 `pipeline`，并保持 `.pri` 接口。

工程收获：版本替换要有稳定接入点，否则每次替换都会波及主工程文件、include 路径和构建配置。

### 问题 6：性能瓶颈在文件中转、重复初始化和访存模式

早期慢点：

- UI 确认后启动子进程，重复初始化渲染环境。
- multiview 生成大量 JPG，再让 elemental 从文件系统读取。
- 270 x 270 = 72900 个 view，150 x 150 target 会产生约 4.58 GiB 数据。
- elemental 目标像素优先的 gather 方式会导致跨帧大步长读取，缓存命中差。

优化方案：

- UI 改成进程内调用 `runHoloPipelineCli`。
- multiview 使用 OSG pbuffer 离屏渲染。
- atlas 自动根据 `GL_MAX_TEXTURE_SIZE` 和页内存预算选择大小。
- `MemoryFrameSink` 保存 multiview 帧，elemental 直接从内存读。
- elemental 输出 materialize 到内存结果，供后续程序消费。
- `elemental_writer_threads=0` 表示自动按硬件线程选择。
- 优化 elemental 内存变换，把访存模式改为更适合缓存的 block/view-major 转置。

关键结果：

```text
早期内存链路测试：
multiview  9.814s
elemental 22.679s
合计      32.493s

最新 pipeline.log：
multiview  7.537s
elemental  2.556s
全流程     16.904s
```

工程收获：性能优化要看数据流。减少 I/O、减少重复初始化、提升内存局部性，常常比盲目加线程更有效。

### 问题 7：Qt 构建链接错误来自工具链混用

用户在 Qt 中遇到：

```text
LNK2019: _Thrd_sleep_for
LNK2019: _Cnd_timedwait_for_unchecked
LNK2019 / LNK2001: __std_find_last_trivial_1
```

分析：

- 这些符号属于 MSVC 标准库实现细节。
- 项目依赖 PCL/Boost `vc142`，而 Qt/对象文件可能混用了其他 MSVC 版本。
- Debug/Release 或不同 toolset 的 `.obj` 混用也会触发类似问题。

解决：

- 固定 Qt Kit 为 `Desktop Qt 5.15.0 MSVC2019 64bit`。
- `Pri/common.pri` 检查 `QMAKE_MSC_VER`，限制在 1920 到 1929。
- 中间文件目录加入 `msvc$QMAKE_MSC_VER`，避免不同工具链对象文件混用。
- 使用 Release 构建。
- 切换工具链后删除 `.qmake.stash`，重新 Run qmake、Clean/Rebuild。

工程收获：C++ Windows 工程里，工具链版本、运行库、第三方库 ABI 必须一致。链接错误不一定是缺代码，也可能是二进制兼容性问题。

### 问题 8：改了 `default_camera.ini` 但运行仍用旧 config

现象：

```text
config camera path: .../config/holoConf-023C
```

后来改为：

```text
config camera path: .../config/084C
pixel size : -1e+10 is error value.
JpIParse Init failed
```

分析路径：

- 先确认 `default_camera.ini` 是否真的被入口读取。
- 检查默认配置查找顺序：源码目录、`00-bin/config/default_camera.ini`、fallback。
- 和可运行的 `holocamera` 对比 SDK 头文件、DLL、config 目录。
- 发现 084C 需要 v4 光场相机 runtime，而不是原 v3.1 路径。

解决：

- `apps/mergeholo_main.cpp` 从 `default_camera.ini` 读取 `camera_config_dir`。
- 默认 fallback 改为 `config/084C`。
- 从 `holocamera/HoloTest/Holo_v4.1.1` 接入 `JpICamera.h`、`JpIParse.h` 和 `JpLFDll-v4.1.1.dll`。
- `build.ps1` 增加 v4 runtime 部署逻辑。
- 真机采集验证生成了 `capture_test_v4_default` 下的 2D JPG 和 3D TIFF。

工程收获：配置问题要区分三层：源码默认值、运行目录配置、实际 SDK/runtime 版本。只改 ini 不一定影响运行，除非入口真的读取它。

## 测试思路

### 测试分层

1. 构建层：确认 qmake、nmake、MSVC toolset、Release 配置、DLL 部署。
2. 启动层：`--mergeholo-help` 检查 exe 和依赖是否能加载。
3. 配置层：`--dry-run` 检查 config 解析、路径解析、阶段开关。
4. 阶段层：单独跑 `depth`、`mesh`、`model`、`multiview`、`elemental`。
5. 纯逻辑层：`test_elemental_memory_transform.cpp` 覆盖内存变换。
6. 性能层：读取 `pipeline.log` 和 benchmark 输出。
7. UI 层：点击拍照、确认、处理、结果输出、按钮状态。
8. 真机层：`--capture --max-frames 1` 验证相机 SDK、配置和实际图像输出。

### 关键测试命令

Release 构建：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\mergeholo\scripts\build.ps1 -Config release
```

启动冒烟：

```powershell
.\mergeholo\00-bin\mergeholo.exe --mergeholo-help
```

管线 dry-run：

```powershell
.\mergeholo\00-bin\mergeholo.exe --pipeline --config .\mergeholo\config\holo_config.microtest.ini --stage all --dry-run
```

UI 生成配置后跑完整 pipeline：

```powershell
.\mergeholo\00-bin\mergeholo.exe --config .\mergeholo\output\holo_config.ui.ini --stage all
```

真机采集验证：

```powershell
.\mergeholo\00-bin\mergeholo.exe --capture --max-frames 1 --duration 15 --no-preview --save-dir .\mergeholo\output\capture_test_v4_default
```

elemental baseline 与 0709 对比：

```powershell
cmd.exe /s /c 'call "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && cd /d C:\wzp\Holographicface\mergeholo\00-bin\elemental_tests && cl /nologo /O2 /MD /EHsc /std:c++17 /utf-8 /DNOMINMAX /DWIN64 ..\..\pipeline\tests\test_elemental_memory_transform.cpp ..\..\pipeline\elemental\ElementalMemoryTransform.cpp /Fe:test_elemental_baseline.exe'

cmd.exe /s /c 'call "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && cd /d C:\wzp\Holographicface\mergeholo\00-bin\elemental_tests && cl /nologo /O2 /MD /EHsc /std:c++17 /utf-8 /DNOMINMAX /DWIN64 ..\..\pipeline\tests\test_elemental_memory_transform.cpp ..\..\pipeline\elemental_0709\ElementalMemoryTransform.cpp /Fe:test_elemental_0709.exe'
```

### elemental 单元测试覆盖点

`pipeline/tests/test_elemental_memory_transform.cpp` 覆盖：

- 输出尺寸计算。
- 默认 270 x 270 views、150 x 150 target 的总字节数。
- 基础像素映射。
- `flipSourceY` 和 `flipViewRows`。
- OpenGL `glReadPixels` bottom-up 行语义。
- source 小于 target 时补零。
- 单线程与多线程输出一致。
- materialized result 的 `copyImage`。
- smoke benchmark 和 medium benchmark。

对比结果：

```text
baseline medium_benchmark_seconds:
0.0789955 / 0.0825592 / 0.0932313

elemental_0709 medium_benchmark_seconds:
0.0516889 / 0.0473662 / 0.045558

两者 checksum 一致，测试均通过。
```

这个测试方式很好：既验证正确性，也验证性能，并且用 checksum 防止只跑快但结果错。

## 修改思路

### 修改顺序

1. 复制和整理项目，而不是先做大重构。
2. 建立构建脚本和依赖部署。
3. 加命令行入口，保证无 UI 也能跑。
4. 加文档，先让项目可理解。
5. 修稳定性问题，比如 PCL 析构和工具链。
6. 加 UI，把操作流程封装起来。
7. 拆模块，让代码结构匹配业务阶段。
8. 抽配置，减少硬编码。
9. 加日志和计时，让性能问题可见。
10. 基于日志逐步优化。
11. 每一步独立提交，效果不好就回退。

### 修改原则

- 旧项目只读，新项目承接修改。
- 先保留能工作的历史逻辑，再逐步替换。
- 每个阶段有明确输入输出。
- 配置文件表达默认参数，代码只负责解析和执行。
- 日志要能说明阶段耗时、内存模式、数据量和结果码。
- 性能优化必须能被测试或日志证明。
- 如果候选方案效果不好，及时回退，不硬合并。

## 用到的工程技术

### C++/Qt 工程

- Qt 5.15 MSVC2019 64-bit。
- qmake `.pro` 和 `.pri` 模块化工程配置。
- Qt Designer `.ui` 文件。
- `QMainWindow`、`QTimer`、`QThread`、`QMessageBox`、`QLabel`、`QProgressBar`。
- OpenCV `cv::Mat` 到 `QImage` 的转换。
- UI 状态机控制按钮可用性。

### 3D/图像/相机

- 光场相机 SDK：`JpICamera`、`JpIParse`、JpLF v4 runtime。
- 2D JPG 和 3D TIFF 采集。
- OpenCV 图像保存、深度图归一化、colormap 预览。
- PCL 点云、法线估计、GreedyProjectionTriangulation、mesh。
- OSG/osgEarth 多视角渲染。
- OpenGL pbuffer 离屏渲染。
- `glReadPixels` GPU 显存读回到内存。
- atlas 分页渲染与 GPU 最大纹理尺寸查询。

### 性能与内存

- `std::unique_ptr` 管理资源生命周期。
- `std::thread`、`std::atomic`、`std::condition_variable`、有界队列。
- 大内存缓冲管理，约 4.58 GiB multiview 和 4.58 GiB elemental output。
- `DepthMemoryResult`、`MeshMemoryResult`、`MultiviewMemoryResult`、`ElementalMemoryResult`。
- 阶段后主动 `clear()`，避免内存长期占用。
- cache-friendly blocked transform。
- 自动线程数和硬件线程探测。
- 阶段计时与 `pipeline.log`。

### Windows 构建与排错

- MSVC v142、Release x64。
- `/MD`、`/MDd`、`/FS`、`/utf-8`、`/arch:AVX`。
- `QMAKE_MSC_VER` 工具链检查。
- PowerShell 构建和部署脚本。
- `dumpbin /dependents` 检查 DLL 依赖。
- `.qmake.stash`、Run qmake、Clean/Rebuild 处理 Qt Creator 工具链切换。
- Git 分支、提交、回退、`diff --no-index` 对比候选版本。

## 这次最值得内化的方法

### 1. 发现问题

不要只看报错文字，要先确定问题属于哪一类：

- 编译错误：头文件、源码、标准版本、编译选项。
- 链接错误：库缺失、工具链 ABI、运行库、Debug/Release 混用。
- 运行崩溃：生命周期、析构顺序、空指针、第三方库内部引用。
- 性能慢：I/O、重复初始化、算法复杂度、缓存命中、GPU readback。
- 配置不生效：入口是否读取、运行目录是否覆盖、fallback 顺序。

### 2. 设计方案

方案设计时先画数据流：

```text
Camera frame
-> frozen cv::Mat
-> output/input/0.jpg + 0.tiff
-> depth memory
-> mesh memory
-> OBJ
-> multiview memory
-> elemental memory
-> pipeline.log
```

然后让代码结构和数据流一致：

```text
camera 负责采集
widgets 负责用户交互
pipeline 负责编排
stages 负责每个处理阶段
config 负责参数
logger/timing 负责证据
```

### 3. 解决问题

每次只动一个方向：

- 要验证工具链，就先不改业务逻辑。
- 要验证配置，就 dry-run。
- 要验证相机，就只采一帧。
- 要验证 elemental，就单独编译小测试。
- 要验证 pipeline，就读 `pipeline.log`。
- 要验证优化，就保留 baseline，对比 benchmark 和 checksum。

### 4. 回退不是失败

这次接入 `pipeline_0708` 后曾经回退，再重接。这个是正确工程动作。

回退说明你有判断标准：如果新方案让构建失败、流程跑不通、性能下降或需求不匹配，就不应该硬往前推。

## 可转化为简历的项目表达

### 简历项目描述

MergeHolo 是一个基于 Qt/C++ 的光场相机采集与 3D 全链路后处理系统，融合历史 `Holo` 点云/网格/多视角/elemental 管线与 `holocamera` 相机采集能力，实现实时 RGB/深度预览、定格拍照、进程内全流程处理、阶段计时日志和内存化数据流。

### 简历 bullet

- 主导将光场相机采集程序与 Holo 3D 后处理管线融合为 `mergeholo`，重构为 `apps/widgets/camera/pipeline/stages/config/ui` 模块化架构，支持实时预览、拍照定格、确认处理和结果输出。
- 将 `multiview -> elemental` 从磁盘 JPG 中转改造为 OSG pbuffer + atlas + 内存帧缓冲链路，减少文件 I/O 和重复加载。
- 设计 `PipelineTiming` 与 `PipelineLogger`，记录 depth、mesh、model、multiview、elemental 阶段耗时、内存模式和数据量，为性能优化提供量化依据。
- 优化 elemental 内存变换，基于 blocked/view-major 访存和自动线程策略，将代表性 pipeline 中 elemental 阶段从约 22.679s 降至约 2.556s。
- 将 UI 确认后的处理从启动子进程改为进程内调用 pipeline，减少渲染环境重复初始化，并保持 UI 状态机和处理线程隔离。
- 解决 PCL mesh 析构崩溃、MSVC 工具链混用链接错误、084C 相机 SDK/runtime 配置不匹配等稳定性问题。
- 建立 Release 构建、dry-run、单元测试、benchmark、真机采集、pipeline 日志的多层测试闭环。

### 面试 STAR 讲法

Situation：原项目由 `Holo` 和 `holocamera` 两套历史代码组成，采集、点云、网格、贴图、多视角和 elemental 流程分散，UI 操作和性能验证不足。

Task：在不破坏旧项目的前提下，新建 `mergeholo`，完成采集与后处理融合，并优化到可交付、可维护、可继续提速的状态。

Action：先建立 qmake/Release 构建和 CLI dry-run，再拆分模块、引入 UI 状态机、抽配置、加日志计时；随后将 multiview 和 elemental 改成内存链路，定位并优化 elemental 访存瓶颈；对工具链、PCL 析构和相机 runtime 做专项排错。

Result：形成模块化 Qt/C++ 工程，完整 pipeline 可在约 16.904s 内完成，elemental 阶段降至约 2.556s，并支持真机采集、UI 操作和阶段日志验证。

## 后续优化建议

目标是缩短到 10 秒数量级，可以继续从这些方向推进：

1. mesh/model 仍约 6.777s，是下一阶段重点。优先确认哪些数据仍落盘，哪些可以继续内存化或缓存。
2. multiview 约 7.537s，其中 GPU render、readback、copy 需要分别计时，考虑减少 readback 次数或优化 atlas page。
3. elemental 已降到约 2.556s，可继续评估 SIMD、NUMA/内存带宽、分块大小、是否必须一次 materialize 4.58 GiB。
4. 如果后续消费者可以流式消费 elemental，可考虑按块产出和消费，减少峰值内存和等待时间。
5. UI 可以继续做进度细分，把 stage timing 实时回传，而不是只显示粗略进度。
6. 相机配置可以进一步抽象成可选设备 profile，例如 `023C`、`084C`、`182C`，避免硬编码。

## 个人能力提升清单

- 遇到大项目融合，先建边界和最小可运行闭环。
- 遇到崩溃，不要用泄漏或跳过析构作为最终方案。
- 遇到慢，先拿日志和 benchmark，再谈优化。
- 遇到链接错误，优先检查工具链、运行库和第三方库 ABI。
- 遇到配置不生效，检查入口读取顺序、运行目录、副本和 fallback。
- 每次大改前保留可回退点。
- 每个优化都要有 baseline、测试命令、结果数据。
- 文档不是事后补作业，而是帮助自己建立项目心智模型。

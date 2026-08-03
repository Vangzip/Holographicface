# MergeHolo

MergeHolo 是把旧项目 `Holo` 的三维重建/多视角/elemental 处理能力，和 `holocamera` 的光场相机采集能力融合后的 Qt/C++ 项目。旧项目只作为来源参考保留，当前可维护代码集中在 `mergeholo` 目录下。

当前主流程面向“一次拍照，一次处理”：

```text
相机实时预览 RGB/深度
-> 拍照定格当前帧
-> 确认后导入 RGB + TIFF
-> depth 生成点云
-> mesh 生成 PCL 网格
-> multiview 直接消费内存 mesh 渲染多视角
-> elemental 从 multiview 内存生成结果
-> 输出日志和结果到 mergeholo/output
```

## 编译环境

推荐固定使用 Release 构建：

```text
Windows x64
Qt 5.15.0 MSVC2019 64-bit
MSVC v142 / VS2019
OpenCV 4.5.0
PCL 1.12.1-rc1
OSG 3.6.5
osgEarth / OE32
CUDA 11.7
JpLFDll-v4.1.1 光场相机 SDK
```

不要用 MinGW，也不要混用 VS2022/VS2026 的对象文件。工程里依赖的 PCL/Boost/Qt 组合按 MSVC2019 x64 配置。

Release 构建命令：

```powershell
cd C:\wzp\Holographicface
powershell -NoProfile -ExecutionPolicy Bypass -File .\mergeholo\scripts\build.ps1 -Config release
```

清理后重建：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\mergeholo\scripts\build.ps1 -Config release -Clean
```

输出程序：

```text
mergeholo/00-bin/mergeholo.exe
```

Qt Creator 中选择 `Desktop Qt 5.15.0 MSVC2019 64bit` Kit，构建配置选择 Release。

## 目录结构

```text
mergeholo/
  apps/                 命令行入口
  camera/               光场相机采集封装
  config/               默认相机、pipeline、打印配置
  docs/                 项目说明、流程说明、agent 上手文档
  pipeline/             Holo 处理管线
    stages/             depth/mesh/model/multiview/elemental 阶段
    multiview/          multiview 内存结果和 PCL mesh -> OSG 构建
    elemental/          elemental 内存处理与 direct sink 实验接口
  printing/             9030 打印相关逻辑
  Pri/                  qmake 依赖配置
  scripts/              构建和测试脚本
  ui/                   Qt Designer .ui 文件
  vendor/               从 Holo/holocamera 合入后仍需维护的第三方/旧模块源码
  widgets/              Qt 窗口和对话框
```

以下目录是生成物或本地历史包，不进入项目源码：

```text
00-bin/      构建输出和运行时 DLL
FF-tmp/      qmake/nmake 中间对象
build/       临时构建目录
debug/       Debug 产物
release/     Release 中间产物
output/      UI/CLI 运行输出
runs/        历史试跑输出
pipeline_*/  已合入或废弃的 pipeline 替换包
```

## UI 用法

直接运行：

```powershell
.\mergeholo\00-bin\mergeholo.exe
```

相机窗口状态：

```text
初始状态：
  实时显示 RGB 和深度图
  拍照按钮可用
  确认、重新拍照不可用

点击拍照：
  当前 RGB/深度帧保存到内存
  预览定格到该帧
  拍照按钮禁用
  确认、重新拍照启用

点击确认：
  RGB/深度写入 output/input
  启动 HoloPipeline
  显示当前进度和日志
  处理期间所有按钮禁用

处理完成：
  结果保存在 mergeholo/output
  elemental 结果保存在内存，可供后续打印/显示逻辑消费

点击重新拍照：
  清除定格帧
  回到实时预览状态
```

## 9030 打印日志

IMC60G 启动诊断会追加写入：

```text
mergeholo/runs/latest/imc60g_startup.log
```

该文件记录启动时的硬件配置、轴映射和诊断路径。9030 打印任务的连接、回零、进度、取消和错误目前通过打印对话框的状态栏与错误栏显示，不会自动生成每次打印的文件日志。

`runs/imc60g-acceptance/*/print_flow.log` 是历史验收输出，不是当前打印模块在运行时自动写入的日志。

## CLI 用法

查看帮助：

```powershell
.\mergeholo\00-bin\mergeholo.exe --mergeholo-help
```

运行完整 pipeline：

```powershell
.\mergeholo\00-bin\mergeholo.exe --config .\mergeholo\output\holo_config.ui.ini --stage all
```

常用阶段：

```text
depth      RGB + TIFF -> 彩色点云
mesh       点云 -> PCL 网格
model      网格 -> OBJ/贴图，当前 all 流程通常跳过
multiview  网格/OBJ -> 多视角内存图
elemental  多视角内存图 -> elemental 内存结果
all        完整处理
mesh-one   单个 PLY 网格处理
```

当前 `all` 流程中，如果 `mesh` 阶段已经在内存里生成了 PCL mesh，`model` 阶段会跳过：

```text
[model] skipped: multiview will consume memory mesh directly.
```

这不是丢掉模型，而是避免 `mesh -> OBJ 落盘 -> OSG 再读 OBJ` 的文件中转，改成：

```text
PCL mesh -> OSG Geometry -> multiview
```

## 配置

默认配置文件位于：

```text
mergeholo/config/default_camera.ini
mergeholo/config/default_pipeline.ini
mergeholo/config/ui_pipeline_template.ini
mergeholo/config/print_9030.ini
```

相机配置最终由 `LightFieldCapture` 读取。UI 运行时会把模板配置复制到 `output/holo_config.ui.ini`，所以如果改了 `00-bin/config/default_camera.ini` 但 UI 仍打印旧路径，需要检查 UI 使用的是哪个模板和输出配置。

相机 084C 示例路径：

```text
mergeholo/00-bin/config/084C
```

如果出现：

```text
pixel size : -1e+10 is error value.
JpIParse Init failed
```

优先对比 `holocamera` 可运行版本的相机 SDK、配置目录内容、DLL 版本和 `JpIParse` 初始化路径。

## 已完成的关键优化

1. PCL 析构崩溃修复  
   Debug/Release 统一 MSVC Runtime，Release 使用 `/MD`，Debug 使用 `/MDd`，两边启用 `/arch:AVX`。恢复 `std::unique_ptr` RAII；`gp3` 析构前断开 input/search tree；`NormalEstimation` 不手动 `setInputCloud(nullptr)`。

2. mesh 单进程处理  
   现在 `-mesh` 只处理一个 PLY，不再为了规避析构崩溃启动多子进程。

3. multiview 内存链路  
   multiview 渲染结果保存在 `MemoryFrameSink` 中，elemental 直接读取内存，不再从文件夹加载多视角 JPG。

4. elemental 线程优化  
   `elemental_writer_threads=0` 时自动使用硬件线程。实测 12 线程场景下 elemental 内存转换约 2.8-3.0 秒。

5. pipeline0710 memory mesh 接入  
   `mesh` 生成的 PCL mesh 直接转换为 OSG 节点给 multiview 使用，跳过 OBJ 生成/读取。一次测试中总耗时从约 22.5 秒降到约 16.8 秒。

6. direct-atlas-elemental 暂不启用  
   0710 的 direct atlas scatter 接口已保留，但默认关闭。当前实现的跨视角散写很慢，测试中 scatter 约 20.7 秒，总流程退化到约 33.8 秒，不适合作为默认路径。

## 当前性能参考

同一份少量 UI 输入数据，Release 构建，`--stage all`：

```text
depth      0.034s
mesh       7.241s
model      0.000s
multiview  6.688s
elemental  2.855s
total     16.818s
```

主要瓶颈仍在：

```text
mesh 法线估计/重建
multiview atlas 渲染和 readback
elemental 大规模内存重排
```

## 维护原则

- 老项目 `Holo`、`holocamera` 只读参考，常规修改放在 `mergeholo`。
- 新的替换版本验证成功后合入 `pipeline/`，不要长期保留 `pipeline_0708`、`pipeline_0709`、`pipeline_0710` 这类目录。
- 运行输出放 `output/` 或 `runs/`，不要提交。
- 手工测试优先使用 Release。
- 若要重新尝试 direct-atlas-elemental，必须先优化 scatter 内存写入方式，再用同一输入和基线对比。

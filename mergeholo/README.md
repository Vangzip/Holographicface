# MergeHolo

`mergeholo` 是把当前工作区里两个旧项目融合后的新 Qt/C++ 项目：

- `Holo`：负责深度图和彩色图到点云、网格、OBJ、多视角图、elemental 图的完整处理管线。
- `holocamera`：负责光场相机采集，获取 2D 图、3D TIFF 深度/坐标图、原始数据等。

旧项目目录 `Holo` 和 `holocamera` 只作为只读来源保留，不在这里修改。新的源码、配置、构建脚本、运行输出都放在 `mergeholo` 下。

## 用途

这个项目的目标是把“相机采集”和“Holo 后处理管线”接成一个可运行流程：

```text
光场相机采集
-> 保存 2D JPG 和 3D TIFF
-> 整理成 Holo 管线输入格式
-> 生成彩色点云 PLY
-> 生成网格 PLY
-> 生成贴图 OBJ
-> 生成多视角 JPG
-> 生成 elemental 目标图
```

它既可以只做相机采集，也可以只跑 Holo 管线，还可以采集完成后自动导入并继续跑完整后处理。

## 编译环境

推荐环境：

```text
Windows x64
Qt 5.15.0 MSVC2019 64-bit
MSVC v142 / x64
OpenCV 4.5.0
PCL 1.12.1-rc1
OSG 3.6.5
osgEarth / OE32
JpLF-v3.1 光场相机 SDK
```

默认 Qt 路径：

```text
C:\wzp\QT\5.15.0\msvc2019_64
```

不建议使用 MinGW 或 32 位 Kit。项目里的 PCL/Boost 库是 `vc142` 版本，`Pri/holo_pipeline.pri` 里已经固定了 Boost toolset 兼容项。

## 目录结构

```text
mergeholo/
  apps/                 统一命令行入口
  camera/               光场相机采集封装，以及从 holocamera 复制来的相机源码
  pipeline/             Holo 管线入口和采集导入桥接逻辑
  vendor/               从 Holo 复制来的点云、多视角、基础工具源码
  config/               Holo 管线配置文件
  Pri/                  qmake 依赖配置
  scripts/              构建和测试脚本
  docs/                 旧项目说明文档快照
  runtime/              可选本地运行时资源目录
  samples/              本地样例输入，默认不提交
  runs/                 采集和管线输出，默认不提交
  00-bin/               构建输出和运行时 DLL，默认不提交
```

## 编译方法

在项目目录运行：

```powershell
cd C:\wzp\Holographicface\mergeholo
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Config release
```

清理后重新编译：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Config release -Clean
```

如果 Qt 或 VS 路径不同，可以手动传入：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 `
  -QtRoot "C:\Qt\5.15.0\msvc2019_64" `
  -VsDevCmd "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
```

Qt Creator 中请使用 `Desktop Qt 5.15.0 MSVC2019 64bit` Kit，编译器选择 VS2019 v142 x64，构建配置选择 Release。从 VS2026 或其他 MSVC 版本切换回来后，先删除 `mergeholo/.qmake.stash`，然后执行 Run qmake，再 Clean/Rebuild。项目会把中间文件放到 `FF-tmp/.../msvc1929`，避免和其他 MSVC 版本的 `.obj` 混用。

编译成功后输出：

```text
mergeholo/00-bin/mergeholo.exe
```

构建脚本会把 Qt、OpenCV、PCL、OSG、OE32、相机 SDK 等运行时 DLL 和配置复制到 `00-bin`。

## 使用方法

查看命令：

```powershell
.\00-bin\mergeholo.exe --mergeholo-help
```

### 1. 只采集相机数据

```powershell
.\00-bin\mergeholo.exe --capture --save-dir .\runs\latest\capture --max-frames 10
```

常用参数：

```text
--save-dir          采集输出目录，默认 runs/latest/capture
--camera-config     相机解析配置目录，默认 00-bin/config/holoConf-023C
--max-frames        最多保存多少帧，0 表示不限制
--duration          最长采集秒数，0 表示不限制
--save-interval-ms  保存间隔，默认 100
--min-free-gb       磁盘可用空间下限，默认 50
--no-preview        不显示 OpenCV 预览窗口
```

采集输出格式：

```text
runs/latest/capture/2d/<timestamp>.jpg
runs/latest/capture/3d/<timestamp>_3D.tiff
runs/latest/capture/raw/
```

### 2. 只导入已采集数据

Holo 管线要求 `.jpg` 和 `.tiff` 同名放在同一个目录。相机采集出来的 3D 文件名带 `_3D` 后缀，所以需要导入整理：

```powershell
.\00-bin\mergeholo.exe --import-capture `
  --capture-dir .\runs\latest\capture `
  --pipeline-input .\runs\latest\pipeline_input
```

导入后格式：

```text
runs/latest/pipeline_input/<timestamp>.jpg
runs/latest/pipeline_input/<timestamp>.tiff
```

### 3. 只跑 Holo 管线

```powershell
.\00-bin\mergeholo.exe --pipeline --config .\config\holo_config.merge.ini --stage all
```

可选阶段：

```text
depth      JPG + TIFF -> 彩色点云 PLY
mesh       点云 PLY -> 网格 PLY
model      网格 PLY + JPG -> 贴图 OBJ
multiview  OBJ -> 多视角 JPG
elemental  多视角 JPG -> elemental JPG
all        完整流程
```

只做配置检查，不真正生成文件：

```powershell
.\00-bin\mergeholo.exe --pipeline --config .\config\holo_config.merge.ini --stage all --dry-run
```

### 4. 采集后自动跑完整流程

```powershell
.\00-bin\mergeholo.exe --capture-and-run --max-frames 1 --stage all
```

这条命令会依次执行：

```text
采集 -> 导入整理 -> Holo 管线
```

默认使用：

```text
采集目录: runs/latest/capture
管线输入: runs/latest/pipeline_input
管线配置: config/holo_config.merge.ini
输出目录: runs/latest/output
```

### 5. 跑本地微测试

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_microtest.ps1 -Stage all -DryRun
```

去掉 `-DryRun` 会真正生成输出：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_microtest.ps1 -Stage all
```

## 核心逻辑

### 入口分发

统一入口在：

```text
apps/mergeholo_main.cpp
```

它根据命令行参数分发到不同模式：

```text
--capture          -> camera/CaptureSession.cpp
--import-capture   -> pipeline/CaptureImport.cpp
--capture-and-run  -> 先 capture，再 import，再 pipeline
--pipeline         -> pipeline/HoloPipeline.cpp
其它 Holo 参数     -> 直接透传给 Holo 管线
```

### 相机采集

采集逻辑在：

```text
camera/CaptureSession.cpp
camera/LightFieldCapture.cpp
```

流程：

```text
初始化 JpLF-v3.1 SDK
-> 读取 config/holoConf-023C 相机解析配置
-> 循环获取 HoloOutData
-> 使用 FrameChangeDetector 判断画面是否有明显变化
-> 保存 2D JPG
-> 保存 3D TIFF
-> 磁盘不足、达到帧数、达到时长或 Ctrl+C 时停止
-> 释放相机资源
```

### 采集导入桥

桥接逻辑在：

```text
pipeline/CaptureImport.cpp
```

它只做文件结构转换，不改变图像内容：

```text
capture/2d/xxx.jpg       -> pipeline_input/xxx.jpg
capture/3d/xxx_3D.tiff   -> pipeline_input/xxx.tiff
```

这样 Holo 原 depth 阶段就能按同名规则找到输入对。

### Holo 管线

管线逻辑在：

```text
pipeline/HoloPipeline.cpp
```

它来自旧 `Holo/main.cpp`，入口从 `main()` 改成了：

```cpp
int runHoloPipelineCli(int argc, char* argv[]);
```

这样 `mergeholo.exe` 可以在同一个进程里调用 Holo 管线，不需要再启动旧项目 exe。

## 关键配置

默认融合配置：

```text
config/holo_config.merge.ini
```

常用配置项：

```ini
depth_input_dir=..\runs\latest\pipeline_input
depth_config=depth_to_pointcloud_config.cfg
mesh_config=mesh_config.cfg
output_root=..\runs\latest\output
multiview_out_dir=multiview
elemental_out_dir=elemental
multiview_angle=90
multiview_per=3
multiview_resolution=150
target_rows=150
target_cols=150
```

含义：

```text
depth_input_dir        Holo 管线输入目录，需要同名 .jpg 和 .tiff
output_root            本次管线输出根目录
multiview_out_dir      多视角图输出目录
elemental_out_dir      elemental 图输出目录
multiview_angle * multiview_per
                       多视角图行列数量
multiview_resolution   每张多视角图分辨率
target_rows/cols       elemental 输出图数量
```

## 运行时资源

相机 SDK 的 `holoLib` 接近 2GB，不直接提交到源码。项目支持两种方式：

```text
mergeholo/runtime/holoLib
..\holocamera\HoloTest\holoLib
```

如果 `mergeholo/runtime/holoLib` 不存在，构建脚本会只读引用旧项目里的：

```text
C:\wzp\Holographicface\holocamera\HoloTest\holoLib
```

相机配置 `holoConf-023C` 会从旧项目发布目录复制到：

```text
mergeholo/00-bin/config/holoConf-023C
```

## 注意事项

- 老项目 `Holo` 和 `holocamera` 不需要修改。
- 新项目所有源码和配置都在 `mergeholo` 下。
- `00-bin`、`runs`、`samples`、`build`、`debug`、`release` 都是本地生成或大文件目录，默认不提交。
- 第一次使用相机前，请确认相机 SDK DLL、驱动、配置和硬件连接正常。
- 全流程 `stage all` 输出量很大，建议先用 `--dry-run` 或 `run_microtest.ps1 -DryRun` 检查配置。
- 如果链接 Boost 报 `vc143` 相关错误，确认使用 MSVC v142/x64，或保留 `Pri/holo_pipeline.pri` 里的 `BOOST_LIB_TOOLSET=\"vc142\"` 配置。

# MergeHolo Qt 项目结构、流程和用法

本文档说明 `mergeholo` Qt 项目的用途、目录结构、编译方式、运行方式、数据流和核心逻辑。`mergeholo` 是在当前工作区中新建的融合项目：把旧 `Holo` 的后处理管线和 `holocamera` 的光场相机采集能力放到一个 Qt/C++ 工程中。旧项目只作为来源和参考，新代码、配置、构建脚本和运行输出都集中在 `mergeholo` 下。

## 1. 项目用途

`mergeholo` 的目标是把以下两段流程接起来：

```text
光场相机采集
-> 保存 2D JPG 和 3D TIFF
-> 整理成 Holo 管线输入
-> 深度图 + 彩图生成彩色点云 PLY
-> 单个 PLY 生成 mesh PLY
-> mesh PLY + JPG 生成贴图 OBJ
-> OBJ 渲染多视角 JPG
-> 多视角 JPG 合成 elemental JPG
```

它支持三类使用方式：

- 只采集相机数据。
- 只导入已有采集数据并跑 Holo 后处理管线。
- 采集完成后自动导入并继续跑后处理。

## 2. 编译环境

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

不要使用 MinGW 或 32 位 Kit。当前 PCL/Boost 库按 MSVC 工具链组织，`Pri/holo_pipeline.pri` 里也固定了：

```text
BOOST_LIB_TOOLSET="vc142"
MSVC CXXFLAGS=/FS /utf-8 /arch:AVX
```

Debug/Release 的 MSVC Runtime 由 Qt mkspec 决定，通常分别是 `/MDd` 和 `/MD`。PCL mesh 析构问题已经通过统一运行库、启用 AVX、恢复 RAII、并在 `gp3` 析构前断开 input/search tree 解决，因此不再需要多子进程规避。

## 3. 目录结构

```text
mergeholo/
  mergeholo.pro                  qmake 主工程文件
  apps/
    mergeholo_main.cpp           程序统一入口和命令分发
  camera/
    CaptureSession.*             相机采集会话，负责保存 JPG/TIFF
    LightFieldCapture.*          JpLF-v3.1 SDK 封装，负责取帧和解析
    FrameChangeDetector.hpp      判断画面是否发生明显变化
    CommonFiles/                 从 holocamera 带来的相机公共头文件
  pipeline/
    CaptureImport.*              把采集输出整理成 Holo 管线输入
    HoloPipeline.*               Holo 后处理管线入口
  vendor/
    base/                        FileLibrary、Logger 等基础工具
    point_cloud/                 深度转点云、mesh、贴图 OBJ 等逻辑
    multiview/                   OSG 多视角渲染控制
  config/
    holo_config.merge.ini        采集到后处理的默认管线配置
    holo_config.example.ini      示例完整配置
    holo_config.microtest.ini    微测试配置
    depth_to_pointcloud_config.cfg
    mesh_config.cfg
  Pri/
    common.pri                   输出目录、临时目录、基础编译选项
    opencv.pri                   OpenCV include/lib
    cuda.pri                     CUDA 可选链接配置
    holo_pipeline.pri            PCL/OSG/OE32 include/lib
  scripts/
    build.ps1                    qmake + nmake 构建和部署脚本
    run_microtest.ps1            本地微测试入口
  docs/                          项目说明文档
  samples/                       样例数据
  runs/                          默认采集和管线输出目录
  runtime/                       可选本地相机 SDK 运行时
  00-bin/                        构建输出和运行时 DLL 部署目录
```

## 4. 主入口分发逻辑

统一入口是：

```text
apps/mergeholo_main.cpp
```

命令分发关系：

```text
mergeholo.exe --capture
  -> camera/CaptureSession.cpp

mergeholo.exe --import-capture
  -> pipeline/CaptureImport.cpp

mergeholo.exe --capture-and-run
  -> 先 runCaptureSession
  -> 再 importCaptureForPipeline
  -> 最后 runHoloPipelineCli

mergeholo.exe --pipeline ...
  -> pipeline/HoloPipeline.cpp

mergeholo.exe --config ...
  -> 直接按 Holo 管线参数处理
```

`projectRoot()` 会根据可执行文件位置寻找 `mergeholo.pro`，因此开发目录运行和 `00-bin` 运行都可以定位默认配置。

## 5. 编译方法

在项目目录运行：

```powershell
cd C:\wzp\Holographicface\mergeholo
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Config release
```

清理后重编译：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Config release -Clean
```

如果 Qt 或 Visual Studio 路径不同：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 `
  -QtRoot "C:\wzp\QT\5.15.0\msvc2019_64" `
  -VsDevCmd "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
```

输出位置：

```text
mergeholo/00-bin/mergeholo.exe
```

构建脚本还会把运行时依赖部署到 `00-bin`，包括 Qt、OpenCV、PCL、VTK、FLANN、Qhull、OpenNI2、OSG、OE32、相机 SDK DLL 和配置。

## 6. 运行方式

查看命令：

```powershell
.\00-bin\mergeholo.exe --mergeholo-help
```

### 6.1 只采集相机数据

```powershell
.\00-bin\mergeholo.exe --capture --save-dir .\runs\latest\capture --max-frames 10
```

常用参数：

```text
--save-dir            采集输出目录，默认 runs/latest/capture
--camera-config       相机解析配置目录，默认 config/holoConf-023C
--max-frames          最多保存多少帧，0 表示不限制
--duration            最长采集秒数，0 表示不限制
--save-interval-ms    保存间隔，默认 100
--min-free-gb         磁盘可用空间下限，默认 50
--no-preview          不显示 OpenCV 预览窗口
```

输出格式：

```text
runs/latest/capture/2d/<timestamp>.jpg
runs/latest/capture/3d/<timestamp>_3D.tiff
runs/latest/capture/raw/
```

### 6.2 导入已采集数据

相机采集输出中 3D 文件名带 `_3D` 后缀，Holo 管线要求同名 `.jpg` 和 `.tiff` 放在同一目录，因此需要导入整理：

```powershell
.\00-bin\mergeholo.exe --import-capture `
  --capture-dir .\runs\latest\capture `
  --pipeline-input .\runs\latest\pipeline_input
```

导入后的格式：

```text
runs/latest/pipeline_input/<timestamp>.jpg
runs/latest/pipeline_input/<timestamp>.tiff
```

### 6.3 只跑 Holo 后处理管线

完整运行：

```powershell
.\00-bin\mergeholo.exe --pipeline --config .\config\holo_config.merge.ini --stage all
```

只检查配置和将要执行的动作，不实际生成文件：

```powershell
.\00-bin\mergeholo.exe --pipeline --config .\config\holo_config.merge.ini --stage all --dry-run
```

可选阶段：

```text
depth      JPG + TIFF -> 彩色点云 PLY
mesh       单个彩色点云 PLY -> mesh PLY
mesh-one   明确处理 --input 指定的单个 PLY
model      mesh PLY + JPG -> 贴图 OBJ
multiview  OBJ -> 多视角 JPG
elemental  多视角 JPG -> elemental JPG
all        按配置顺序运行完整流程
```

### 6.4 单个 PLY 生成 mesh

当前 `mesh` 阶段只处理一个 PLY，不再做多文件扫描批处理，也不再启用多子进程。

推荐显式指定输入：

```powershell
.\00-bin\mergeholo.exe --pipeline `
  --config .\config\holo_config.merge.ini `
  --stage mesh `
  --input .\runs\latest\pipeline_input\0_rgb.ply
```

行为规则：

```text
带 --input:
  只处理指定的一个 PLY。

不带 --input:
  如果 depth_input_dir 下只有一个 *_rgb.ply，则处理这个文件。
  如果 depth_input_dir 下有多个 *_rgb.ply，则报错并要求传 --input。

不再使用:
  mesh_worker_processes
  mesh_worker_log_dir
  mesh worker 子进程池
```

### 6.5 采集后自动跑完整管线

```powershell
.\00-bin\mergeholo.exe --capture-and-run --max-frames 1 --stage all
```

默认路径：

```text
采集目录:   runs/latest/capture
管线输入:   runs/latest/pipeline_input
管线配置:   config/holo_config.merge.ini
输出目录:   runs/latest/output
```

`--capture-and-run` 当前只把 `--config` 和 `--stage` 传给管线。如果要给 mesh 指定 `--input`，建议分两步运行：先采集/导入，再用 `--pipeline --stage mesh --input ...`。

## 7. Holo 管线内部流程

核心文件：

```text
pipeline/HoloPipeline.cpp
```

配置读取：

```text
holo_config.merge.ini
  depth_input_dir
  depth_config
  mesh_config
  mesh_obj
  output_root
  multiview_out_dir
  elemental_out_dir
  model_type
  multiview_*
  target_rows / target_cols
  run_depth_pointcloud / run_mesh / run_textured_model / run_multiview / run_elemental
```

阶段执行：

```text
runPipeline
  -> runDepthStage
  -> runMeshStage
  -> runModelStage
  -> runMultiviewStage
  -> runElementalStage
```

每个阶段都会被 `runTimedStage` 包裹，结束时打印耗时汇总。

### 7.1 depth 阶段

入口：

```text
runDepthStage
```

逻辑：

```text
扫描 depth_input_dir 下的 .tiff
-> 找同名 .jpg
-> depthToPlyColor(depth, jpg, depth_config, depth_input_dir)
-> 生成 <stem>_rgb.ply
```

主要实现：

```text
vendor/point_cloud/src/depth_io.cpp
vendor/point_cloud/include/depth_io.h
```

### 7.2 mesh 阶段

入口：

```text
runMeshStage
runSingleMeshInput
runMeshOneStage
```

逻辑：

```text
确定一个输入 PLY
-> ConverPointCloud::meshAPI(input_ply, mesh_config, depth_input_dir)
-> 按 mesh_config 选择重建方式
-> 当前配置默认 GreedyProjectionTriangulation
-> 生成 <stem>_mesh.ply
```

重要点：

- 只处理一个 PLY。
- 多个 PLY 时必须用 `--input` 指定。
- PCL 析构问题已经修复，不再需要子进程隔离。
- `gp3` 析构前会断开 input cloud 和 search tree。
- `NormalEstimation` 不手动 `setInputCloud(nullptr)`，直接析构。

主要实现：

```text
vendor/point_cloud/src/ConverPointCloud.cpp
vendor/point_cloud/include/ConverPointCloud.h
```

### 7.3 model 阶段

入口：

```text
runModelStage
```

逻辑：

```text
扫描 depth_input_dir 下的 *_mesh.ply
-> ConverPointCloud::modelAPI(mesh_ply, mesh_config)
-> 找对应 JPG 贴图
-> 生成 <stem>_mesh_model.obj / .mtl / .jpg
```

主要实现：

```text
vendor/point_cloud/src/OdmTexturing.cpp
vendor/point_cloud/include/OdmTexturing.hpp
```

### 7.4 multiview 阶段

入口：

```text
runMultiviewStage
```

逻辑：

```text
读取 mesh_obj
-> 建 OSG Viewer 和 GraphicsContext
-> 加载 OBJ
-> modelMoveHandler 控制模型姿态/相机
-> 每个视角渲染一张 JPG
-> 输出到 multiview_out_dir
```

默认视角数量：

```text
viewRows = multiview_angle * multiview_per
viewCols = viewRows
```

例如默认：

```text
multiview_angle=90
multiview_per=3
=> 270 x 270 张视角图
```

主要实现：

```text
vendor/multiview/modelMoveHandler.cpp
vendor/multiview/modelMoveHandler.h
```

### 7.5 elemental 阶段

入口：

```text
runElementalStage
```

逻辑：

```text
读取 multiview_out_dir 中所有视角图
-> 将每个视角图像素缓存到内存
-> 对目标平面每个像素重排视角
-> 输出 target_rows * target_cols 张 elemental JPG
```

默认输出：

```text
target_rows=150
target_cols=150
=> 22500 张 elemental 图
```

输出图尺寸：

```text
viewCols x viewRows
默认 270 x 270
```

## 8. 相机采集流程

核心文件：

```text
camera/CaptureSession.cpp
camera/LightFieldCapture.cpp
camera/FrameChangeDetector.hpp
```

流程：

```text
runCaptureSession
  -> 创建输出目录 2d / 3d / raw
  -> 初始化 LightFieldCapture
  -> LightFieldCapture::initialize
       -> JpICamera::GetICamera
       -> ICam->Init
       -> JpIParse::GetIParse
       -> IParse->Init(camera config)
       -> StartCapture
  -> 后台线程循环 ICam->Capture
  -> IParse->Parse
  -> HoloOutData 入队
  -> 主线程取队列数据
  -> 预览 2D/3D
  -> FrameChangeDetector 判断是否保存
  -> 保存 2d/<timestamp>.jpg
  -> 保存 3d/<timestamp>_3D.tiff
  -> 达到帧数、时长、磁盘阈值或 Ctrl+C 后退出
  -> release 相机资源
```

`CaptureSession` 做了退出保护：

- 注册 `atexit` 清理。
- Windows 下注册 `SetConsoleCtrlHandler`。
- 收到 Ctrl+C、关闭窗口、系统退出等事件时尽量释放相机 SDK 资源。

## 9. 数据文件命名约定

采集输出：

```text
capture/2d/0706_170000123.jpg
capture/3d/0706_170000123_3D.tiff
```

导入后：

```text
pipeline_input/0706_170000123.jpg
pipeline_input/0706_170000123.tiff
```

Holo 管线中间产物：

```text
pipeline_input/0706_170000123_rgb.ply
pipeline_input/0706_170000123_mesh.ply
pipeline_input/0706_170000123_mesh_model.obj
pipeline_input/0706_170000123_mesh_model.mtl
pipeline_input/0706_170000123_mesh_model.jpg
```

多视角和 elemental 输出：

```text
runs/latest/output/multiview/<row><col>.jpg
runs/latest/output/elemental/<row><col>.jpg
```

## 10. 关键配置文件

### 10.1 `config/holo_config.merge.ini`

用于采集到后处理的默认流程。

关键项：

```ini
depth_input_dir=..\runs\latest\pipeline_input
depth_config=depth_to_pointcloud_config.cfg
mesh_config=mesh_config.cfg
mesh_obj=
output_root=..\runs\latest\output

run_depth_pointcloud=true
run_mesh=true
run_textured_model=true
run_multiview=true
run_elemental=true
```

`mesh_obj` 为空时，multiview 阶段会在 `depth_input_dir` 下自动找第一个 OBJ。完整流水线一般由 model 阶段生成 OBJ 后继续渲染。

### 10.2 `config/mesh_config.cfg`

控制点云到 mesh、贴图等参数。当前默认：

```ini
reconstruct=2
```

含义：

```text
1 = Poisson reconstruction
2 = GreedyProjectionTriangulation
```

GreedyProjectionTriangulation 是当前主要路径。

### 10.3 `config/depth_to_pointcloud_config.cfg`

控制深度图/TIFF 到彩色点云 PLY 的参数，如点云采样、缩放、坐标转换等。

## 11. 常见开发修改点

修改命令入口：

```text
apps/mergeholo_main.cpp
```

修改采集参数默认值：

```text
camera/CaptureSession.cpp
CaptureSessionOptions in camera/CaptureSession.h
```

修改相机 SDK 初始化、取帧、解析：

```text
camera/LightFieldCapture.cpp
camera/LightFieldCapture.h
```

修改导入规则：

```text
pipeline/CaptureImport.cpp
```

修改 Holo 管线阶段：

```text
pipeline/HoloPipeline.cpp
```

修改点云、mesh、OBJ 生成：

```text
vendor/point_cloud/src/ConverPointCloud.cpp
vendor/point_cloud/src/depth_io.cpp
vendor/point_cloud/src/OdmTexturing.cpp
vendor/point_cloud/src/poissonmesh.cpp
```

修改多视角渲染姿态/命名/截图：

```text
vendor/multiview/modelMoveHandler.cpp
vendor/multiview/modelMoveHandler.h
```

修改依赖路径：

```text
Pri/opencv.pri
Pri/cuda.pri
Pri/holo_pipeline.pri
mergeholo.pro
scripts/build.ps1
```

## 12. 测试和排查

微测试：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_microtest.ps1 -Stage all -DryRun
```

真实跑微测试：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_microtest.ps1 -Stage all
```

只验证单 PLY mesh：

```powershell
.\00-bin\mergeholo.exe --pipeline `
  --config .\config\holo_config.merge.ini `
  --stage mesh `
  --input .\samples\face_roate\0_rgb.ply
```

如果 mesh 阶段报多个 PLY：

```text
[mesh] found N *_rgb.ply files; pass --input to choose one.
```

处理方式是显式加 `--input`，或者把 `depth_input_dir` 中多余的 `*_rgb.ply` 移走。

如果启动时报相机 SDK 找不到：

```text
JpLF-v3.1 SDK not found
```

处理方式：

- 设置环境变量 `HOLO_SDK_ROOT` 指向包含 `JpLF-v3.1.lib` 的目录。
- 或保留 `mergeholo/runtime/holoLib`。
- 或保留旧 `holocamera/HoloTest/holoLib` 作为 fallback。

如果运行时报 DLL 找不到，先重新部署：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Config release
```

## 13. 当前重要约束

- `mesh` 阶段只处理一个 PLY。
- `mesh_worker_processes` 和多子进程 mesh 已移除。
- `multiview` 当前只支持 `model_type=obj`。
- `capture-and-run` 不会自动传递 `--input` 给 mesh；如果需要指定单个 PLY，建议分步运行。
- 旧 `Holo` 和 `holocamera` 目录作为来源参考，不在融合项目开发中直接修改。

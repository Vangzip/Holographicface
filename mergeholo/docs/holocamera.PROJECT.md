# holocamera 项目说明

## 项目功能

`holocamera` 是一个基于 Qt 5 / MSVC / OpenCV 的光场相机采集 Demo。当前默认入口是 `HoloTest/main_holo_capture.cpp`，程序会初始化光场相机，持续获取 2D 图像、3D 图像、深度图和原始数据，并按帧变化检测结果保存有效帧。

核心行为：

- 初始化 `JpLF-v3.1` 光场相机库并读取 `config/holoConf-023C` 解析配置。
- 显示实时 `holo 2d` 和 `holo 3d` 窗口。
- 使用 `FrameChangeDetector.hpp` 判断两帧之间是否有显著变化。
- 默认把 2D JPG 和 3D TIFF 保存到 `D:/HoloImages/`。
- 低磁盘空间时自动停止，默认阈值为 50 GB。
- 支持 Ctrl+C / 控制台关闭时释放相机资源。

## 目录结构

```text
holocamera/
  HoloTest/                 Qt/qmake 主工程
    HoloTest.pro            qmake 工程文件
    main_holo_capture.cpp   当前默认采集入口
    LightFieldCapture.*     光场相机封装
    FrameChangeDetector.hpp 帧变化检测
    CommonFiles/            公共工具、配置、数据库/Qt/OpenCV 辅助代码
    holoLib/                相机 SDK、运行库、配置、模型资源
  Pri/                      qmake 依赖配置
    common.pri              输出目录与通用编译选项
    opencv.pri              OpenCV 路径与链接配置
    cuda.pri                CUDA 可选链接配置
  scripts/
    build.ps1               一键构建脚本
    create-github-repo.ps1  使用 GitHub CLI 创建远端仓库
    push-with-timestamp.ps1 每次推送到带时间后缀的远端分支
  00-bin/                   构建输出目录，生成 HoloTest.exe
  FF-tmp/                   qmake/uic/moc/obj 临时目录
```

## 本机已验证环境

- Windows
- Visual Studio 2026 Community，MSVC x64 工具链
- Qt 5.15.0 MSVC2019 x64：`C:/wzp/QT/5.15.0/msvc2019_64`
- OpenCV 4.5.0：自动发现 `C:/wzp/Holographicface/opencv450/opencv/build`
- Git LFS 3.7.1
- GitHub CLI 已安装，但当前未登录

CUDA 未作为必需项处理：`Pri/cuda.pri` 会优先读取 `CUDA_PATH`，找不到 CUDA 时只告警并跳过 CUDA 链接库。

## 构建方法

在项目根目录运行：

```powershell
.\scripts\build.ps1 -Config release -Clean
```

成功后会生成：

```text
00-bin/HoloTest.exe
```

如果 Qt 或 Visual Studio 安装路径不同：

```powershell
.\scripts\build.ps1 -QtRoot "C:\Qt\5.15.0\msvc2019_64" -VsDevCmd "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
```

## 运行方法

构建成功后运行：

```powershell
.\00-bin\HoloTest.exe
```

运行前确认：

- 相机 SDK DLL、`JpLF-v3.1.dll`、OpenCV/Qt DLL 位于 exe 可加载路径中。
- `00-bin/config/holoConf-023C` 或发布目录中的相机配置存在。
- `D:/HoloImages/` 所在磁盘有足够空间；默认低于 50 GB 时会停止。

## GitHub 管理

本项目包含很多超过 GitHub 普通 Git 100 MiB 限制的 DLL、模型和压缩包，所以已配置 `.gitattributes` 使用 Git LFS 管理大文件。首次提交前执行：

```powershell
git lfs install
git add .gitattributes
```

创建 GitHub 仓库需要先登录：

```powershell
gh auth login
```

登录后创建私有仓库：

```powershell
.\scripts\create-github-repo.ps1 -Name holocamera -Visibility private
```

每次推送到带时间后缀的远端分支：

```powershell
.\scripts\push-with-timestamp.ps1 -BranchPrefix main -PushTag
```

示例远端分支名：

```text
main-20260703-161530
```

示例 tag：

```text
snapshot-20260703-161530
```

## 当前注意事项

- `HoloTest/main_holo_capture.cpp` 中保存目录目前写死为 `D:/HoloImages/`，后续建议改成配置文件或命令行参数。
- `JpICamera.h` 编译时有源字符集警告，说明该文件可能不是 UTF-8；目前不影响 Release 构建。
- `00-bin - 副本/` 和根目录 `.7z` 是重复发布包/归档，已默认忽略，不建议直接进 Git。

@echo off
chcp 65001 >nul
echo ========================================
echo Face_Roate 文件夹处理批处理脚本
echo ========================================
echo.

REM 设置路径（请根据实际情况修改）
set "PROGRAM_PATH=..\target\point_cloud.exe"
set "FACE_ROATE_DIR=%~dp0"
if "%FACE_ROATE_DIR:~-1%"=="\" set "FACE_ROATE_DIR=%FACE_ROATE_DIR:~0,-1%"
set "DEPTH_CONFIG=%FACE_ROATE_DIR%\depth_to_pointcloud_config.cfg"
set "MESH_CONFIG=%FACE_ROATE_DIR%\mesh_config.cfg"



REM 检查程序是否存在
if not exist "%PROGRAM_PATH%" (
    echo 错误: 找不到程序文件: %PROGRAM_PATH%
    echo 请修改脚本中的 PROGRAM_PATH 变量
    pause
    exit /b 1
)

echo 步骤1: 深度图转点云
echo ----------------------------------------
"%PROGRAM_PATH%" -point "%FACE_ROATE_DIR%" -config "%DEPTH_CONFIG%"
if errorlevel 1 (
    echo 步骤1失败，请检查配置文件和输入文件
    pause
    exit /b 1
)
echo.

echo 步骤2: 点云转网格
echo ----------------------------------------
"%PROGRAM_PATH%" -mesh "%FACE_ROATE_DIR%" -config "%MESH_CONFIG%"
if errorlevel 1 (
    echo 步骤2失败，请检查配置文件
    pause
    exit /b 1
)
echo.

echo 步骤3: 网格贴图
echo ----------------------------------------
"%PROGRAM_PATH%" -model "%FACE_ROATE_DIR%" -config "%MESH_CONFIG%"
if errorlevel 1 (
    echo 步骤3失败，请检查配置文件
    pause
    exit /b 1
)
echo.

echo ========================================
echo 处理完成！
echo ========================================
pause




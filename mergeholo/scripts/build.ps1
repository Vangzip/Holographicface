param(
    [ValidateSet("release", "debug")]
    [string]$Config = "release",
    [string]$QtRoot = "",
    [string]$VsDevCmd = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat",
    [switch]$Clean,
    [switch]$SkipDeploy
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$WorkspaceRoot = Split-Path -Parent $ProjectRoot
if (-not $QtRoot) {
    $QtRoot = Join-Path $WorkspaceRoot "..\QT\5.15.0\msvc2019_64"
    $QtRoot = [System.IO.Path]::GetFullPath($QtRoot)
}

$QMake = Join-Path $QtRoot "bin\qmake.exe"
if (-not (Test-Path -LiteralPath $QMake)) {
    throw "qmake not found: $QMake. Install Qt 5.15 MSVC2019 x64 or pass -QtRoot."
}
if (-not (Test-Path -LiteralPath $VsDevCmd)) {
    throw "Visual Studio developer command script not found: $VsDevCmd"
}

$Makefile = if ($Config -eq "debug") { "Makefile.Debug" } else { "Makefile.Release" }
$QMakeConfig = "CONFIG+=$Config"
$OutDir = Join-Path $ProjectRoot "00-bin"

function Copy-RuntimeItem {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    if (Test-Path -LiteralPath $Source -PathType Container) {
        if (-not (Test-Path -LiteralPath $Destination)) {
            New-Item -ItemType Directory -Path $Destination | Out-Null
        }
        Get-ChildItem -LiteralPath $Source -Force | ForEach-Object {
            Copy-RuntimeItem -Source $_.FullName -Destination (Join-Path $Destination $_.Name)
        }
        return
    }

    if (Test-Path -LiteralPath $Source -PathType Leaf) {
        $Parent = Split-Path -Parent $Destination
        if ($Parent -and -not (Test-Path -LiteralPath $Parent)) {
            New-Item -ItemType Directory -Path $Parent | Out-Null
        }
        Copy-Item -LiteralPath $Source -Destination $Destination -Force
    }
}

function Copy-FileSet {
    param(
        [Parameter(Mandatory = $true)][string]$SourceDir,
        [Parameter(Mandatory = $true)][string]$DestinationDir,
        [Parameter(Mandatory = $true)][string[]]$Patterns
    )

    if (-not (Test-Path -LiteralPath $SourceDir)) {
        return
    }
    if (-not (Test-Path -LiteralPath $DestinationDir)) {
        New-Item -ItemType Directory -Path $DestinationDir | Out-Null
    }
    foreach ($Pattern in $Patterns) {
        Get-ChildItem -LiteralPath $SourceDir -File -Filter $Pattern -ErrorAction SilentlyContinue | ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $DestinationDir $_.Name) -Force
        }
    }
}

function Deploy-HoloPipelineRuntime {
    param([Parameter(Mandatory = $true)][string]$DestinationRoot)

    $RuntimeRoots = @(
        (Join-Path $WorkspaceRoot "opencv450\opencv\build\x64\vc15\bin"),
        (Join-Path $WorkspaceRoot "PCL 1.12.1-rc1\bin"),
        (Join-Path $WorkspaceRoot "PCL 1.12.1-rc1\3rdParty\VTK\bin"),
        (Join-Path $WorkspaceRoot "PCL 1.12.1-rc1\3rdParty\FLANN\bin"),
        (Join-Path $WorkspaceRoot "PCL 1.12.1-rc1\3rdParty\Qhull\bin"),
        (Join-Path $WorkspaceRoot "PCL 1.12.1-rc1\3rdParty\OpenNI2\Redist"),
        (Join-Path $WorkspaceRoot "PCL 1.12.1-rc1\3rdParty\OpenNI2\Tools"),
        (Join-Path $WorkspaceRoot "OSG365\bin"),
        (Join-Path $WorkspaceRoot "OE32\bin")
    )

    foreach ($RuntimeRoot in $RuntimeRoots) {
        Copy-FileSet -SourceDir $RuntimeRoot -DestinationDir $DestinationRoot -Patterns @("*.dll")
    }

    $LegacyHoloTarget = Join-Path $WorkspaceRoot "Holo\target"
    Copy-FileSet -SourceDir $LegacyHoloTarget -DestinationDir $DestinationRoot -Patterns @("zlib.dll")

    foreach ($PluginRoot in @(
        (Join-Path $WorkspaceRoot "OSG365\bin\osgPlugins-3.6.5"),
        (Join-Path $WorkspaceRoot "OE32\bin\osgPlugins-3.6.5")
    )) {
        if (Test-Path -LiteralPath $PluginRoot) {
            Copy-RuntimeItem -Source $PluginRoot -Destination (Join-Path $DestinationRoot "osgPlugins-3.6.5")
        }
    }
}

function Deploy-CameraRuntime {
    param([Parameter(Mandatory = $true)][string]$DestinationRoot)

    $HoloLib = $env:HOLO_SDK_ROOT
    if (-not $HoloLib) {
        $LocalRuntime = Join-Path $ProjectRoot "runtime\holoLib"
        $LegacyRuntime = Join-Path $WorkspaceRoot "holocamera\HoloTest\holoLib"
        if (Test-Path -LiteralPath (Join-Path $LocalRuntime "JpLF-v3.1.lib")) {
            $HoloLib = $LocalRuntime
        } elseif (Test-Path -LiteralPath (Join-Path $LegacyRuntime "JpLF-v3.1.lib")) {
            $HoloLib = $LegacyRuntime
        }
    }
    if (-not $HoloLib -or -not (Test-Path -LiteralPath $HoloLib)) {
        Write-Warning "Camera SDK runtime not found. Set HOLO_SDK_ROOT or keep holocamera/HoloTest/holoLib available."
        return
    }

    Copy-FileSet -SourceDir $HoloLib -DestinationDir $DestinationRoot -Patterns @("*.dll", "*.cti", "*.ini", "*.bin", "*.yaml", "*.txt", "*.db")
    foreach ($DirName in @("Drivers", "iconengines", "imageformats", "maskDetect", "onnx_model", "platforms", "sqldrivers", "styles")) {
        $SourceDir = Join-Path $HoloLib $DirName
        if (Test-Path -LiteralPath $SourceDir) {
            Copy-RuntimeItem -Source $SourceDir -Destination (Join-Path $DestinationRoot $DirName)
        }
    }

    $ConfigTarget = Join-Path $DestinationRoot "config"
    if (-not (Test-Path -LiteralPath $ConfigTarget)) {
        New-Item -ItemType Directory -Path $ConfigTarget | Out-Null
    }
    $LegacyCameraConfig = Join-Path $WorkspaceRoot "holocamera\00-bin\config\holoConf-023C"
    if (Test-Path -LiteralPath $LegacyCameraConfig) {
        Copy-RuntimeItem -Source $LegacyCameraConfig -Destination (Join-Path $ConfigTarget "holoConf-023C")
    }

    $HoloLibConfig = Join-Path $HoloLib "config"
    if (Test-Path -LiteralPath $HoloLibConfig) {
        foreach ($ConfigItem in @("182C", "database.db", "database_2024.db", "pre_database.db")) {
            $Source = Join-Path $HoloLibConfig $ConfigItem
            if (Test-Path -LiteralPath $Source) {
                Copy-RuntimeItem -Source $Source -Destination (Join-Path $ConfigTarget $ConfigItem)
            }
        }
    }
}

function Deploy-MergeHoloConfig {
    param([Parameter(Mandatory = $true)][string]$DestinationRoot)

    $ConfigTarget = Join-Path $DestinationRoot "config"
    if (-not (Test-Path -LiteralPath $ConfigTarget)) {
        New-Item -ItemType Directory -Path $ConfigTarget | Out-Null
    }
    Copy-FileSet -SourceDir (Join-Path $ProjectRoot "config") -DestinationDir $ConfigTarget -Patterns @("*.ini", "*.cfg")
}

Push-Location $ProjectRoot
try {
    $Cmd = "call `"$VsDevCmd`" -arch=x64 && `"$QMake`" mergeholo.pro -spec win32-msvc $QMakeConfig"
    if ($Clean) {
        $Cmd += " && if exist $Makefile nmake /f $Makefile clean"
    }
    $Cmd += " && nmake /f $Makefile"
    cmd.exe /s /c $Cmd
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE"
    }

    if (-not $SkipDeploy.IsPresent) {
        Write-Host "Deploying runtime dependencies to $OutDir"
        if (-not (Test-Path -LiteralPath $OutDir)) {
            New-Item -ItemType Directory -Path $OutDir | Out-Null
        }
        Deploy-HoloPipelineRuntime -DestinationRoot $OutDir
        Deploy-CameraRuntime -DestinationRoot $OutDir
        Deploy-MergeHoloConfig -DestinationRoot $OutDir
    }
}
finally {
    Pop-Location
}

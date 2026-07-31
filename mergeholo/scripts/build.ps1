param(
    [ValidateSet("release", "debug")]
    [string]$Config = "release",
    [string]$QtRoot = "",
    [string]$VsDevCmd = "",
    [switch]$Clean,
    [switch]$SkipDeploy
)

$ErrorActionPreference = "Stop"

function Resolve-Vs2019DevCmd {
    $VsWhereCandidates = @()
    if (${env:ProgramFiles(x86)}) {
        $VsWhereCandidates += (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe")
    }
    if ($env:ProgramFiles) {
        $VsWhereCandidates += (Join-Path $env:ProgramFiles "Microsoft Visual Studio\Installer\vswhere.exe")
    }

    foreach ($VsWhere in $VsWhereCandidates) {
        if (-not (Test-Path -LiteralPath $VsWhere)) {
            continue
        }

        $InstallPath = & $VsWhere -version "[16.0,17.0)" -products * -property installationPath 2>$null |
            Select-Object -First 1
        if ($InstallPath) {
            $Candidate = Join-Path $InstallPath "Common7\Tools\VsDevCmd.bat"
            if (Test-Path -LiteralPath $Candidate) {
                return $Candidate
            }
        }
    }

    foreach ($Candidate in @(
        "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat"
    )) {
        if (Test-Path -LiteralPath $Candidate) {
            return $Candidate
        }
    }

    return ""
}

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$WorkspaceRoot = Split-Path -Parent $ProjectRoot
if (-not $QtRoot) {
    $QtRoot = Join-Path $WorkspaceRoot "..\QT\5.15.0\msvc2019_64"
    $QtRoot = [System.IO.Path]::GetFullPath($QtRoot)
}
if (-not $VsDevCmd) {
    $VsDevCmd = Resolve-Vs2019DevCmd
}

$QMake = Join-Path $QtRoot "bin\qmake.exe"
if (-not (Test-Path -LiteralPath $QMake)) {
    throw "qmake not found: $QMake. Install Qt 5.15 MSVC2019 x64 or pass -QtRoot."
}
if (-not (Test-Path -LiteralPath $VsDevCmd)) {
    throw "Visual Studio 2019 developer command script not found. Install VS2019 with MSVC v142 x64 tools or pass -VsDevCmd."
}

$Makefile = if ($Config -eq "debug") { "Makefile.Debug" } else { "Makefile.Release" }
$QMakeConfig = "CONFIG+=$Config"
$OutDir = Join-Path $ProjectRoot "00-bin"
$repoRoot = $ProjectRoot
$outputDir = $OutDir

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

    $JpLfV4Root = $env:JP_LF_V4_ROOT
    if (-not $JpLfV4Root) {
        $Candidate = Join-Path $WorkspaceRoot "holocamera\HoloTest\Holo_v4.1.1"
        if (Test-Path -LiteralPath (Join-Path $Candidate "windows\JpLFDll-v4.1.1.dll")) {
            $JpLfV4Root = $Candidate
        }
    }
    if (-not $JpLfV4Root -or -not (Test-Path -LiteralPath (Join-Path $JpLfV4Root "windows\JpLFDll-v4.1.1.dll"))) {
        Write-Warning "JpLFDll-v4.1.1 runtime not found. Set JP_LF_V4_ROOT or keep holocamera/HoloTest/Holo_v4.1.1 available."
    } else {
        Copy-FileSet -SourceDir (Join-Path $JpLfV4Root "windows") -DestinationDir $DestinationRoot -Patterns @("JpLFDll-v4.1.1.dll")
    }

    $CameraRuntime = $env:HOLO_CAMERA_RUNTIME
    if (-not $CameraRuntime) {
        $Candidate = Join-Path $WorkspaceRoot "holocamera\00-bin"
        if (Test-Path -LiteralPath $Candidate) {
            $CameraRuntime = $Candidate
        }
    }
    if (-not $CameraRuntime -or -not (Test-Path -LiteralPath $CameraRuntime)) {
        Write-Warning "Camera runtime root not found. Set HOLO_CAMERA_RUNTIME or keep holocamera/00-bin available."
        return
    }

    Copy-FileSet -SourceDir $CameraRuntime -DestinationDir $DestinationRoot -Patterns @("*.dll", "*.cti", "*.ini", "*.bin", "*.yaml", "*.txt", "*.db")
    foreach ($DirName in @("Camera", "Drivers", "iconengines", "imageformats", "maskDetect", "onnx_model", "platforms", "sqldrivers", "styles")) {
        $SourceDir = Join-Path $CameraRuntime $DirName
        if (Test-Path -LiteralPath $SourceDir) {
            Copy-RuntimeItem -Source $SourceDir -Destination (Join-Path $DestinationRoot $DirName)
        }
    }

    $ConfigTarget = Join-Path $DestinationRoot "config"
    if (-not (Test-Path -LiteralPath $ConfigTarget)) {
        New-Item -ItemType Directory -Path $ConfigTarget | Out-Null
    }

    $CameraConfig = Join-Path $CameraRuntime "config"
    if (Test-Path -LiteralPath $CameraConfig) {
        foreach ($ConfigItem in @("084C", "182C", "holoConf-023C")) {
            $Source = Join-Path $CameraConfig $ConfigItem
            if (Test-Path -LiteralPath $Source) {
                Copy-RuntimeItem -Source $Source -Destination (Join-Path $ConfigTarget $ConfigItem)
            }
        }
        Copy-FileSet -SourceDir $CameraConfig -DestinationDir $ConfigTarget -Patterns @("*.db", "*.json")
    }
}

function Deploy-MsvcRuntime {
    param(
        [Parameter(Mandatory = $true)][string]$DestinationRoot,
        [Parameter(Mandatory = $true)][string]$VsDevCmdPath
    )

    $VsRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $VsDevCmdPath))
    $RedistRoot = Join-Path $VsRoot "VC\Redist\MSVC"
    if (-not (Test-Path -LiteralPath $RedistRoot)) {
        Write-Warning "MSVC redist root not found: $RedistRoot"
        return
    }

    $RedistDir = Get-ChildItem -LiteralPath $RedistRoot -Directory |
        Sort-Object Name -Descending |
        ForEach-Object {
            Get-ChildItem -LiteralPath (Join-Path $_.FullName "x64") -Directory -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -like "Microsoft.VC*.CRT" } |
                Select-Object -First 1
        } |
        Select-Object -First 1

    if (-not $RedistDir) {
        Write-Warning "MSVC x64 redist directory not found under: $RedistRoot"
        return
    }

    Copy-FileSet -SourceDir $RedistDir.FullName -DestinationDir $DestinationRoot -Patterns @(
        "concrt140.dll",
        "msvcp140*.dll",
        "vcruntime140*.dll"
    )
}

function Deploy-MergeHoloConfig {
    param([Parameter(Mandatory = $true)][string]$DestinationRoot)

    $ConfigTarget = Join-Path $DestinationRoot "config"
    if (-not (Test-Path -LiteralPath $ConfigTarget)) {
        New-Item -ItemType Directory -Path $ConfigTarget | Out-Null
    }
    Copy-FileSet -SourceDir (Join-Path $ProjectRoot "config") -DestinationDir $ConfigTarget -Patterns @("*.ini", "*.cfg")
}

function Remove-ObsoletePrintRuntime {
    param([Parameter(Mandatory = $true)][string]$DestinationRoot)

    foreach ($FileName in @(
        "DfjzhControlerDll.dll",
        "dfjzh6052dll.dll",
        "dfjzh6052dll0.dll",
        "CH365DLL.DLL"
    )) {
        $LegacyPath = Join-Path $DestinationRoot $FileName
        if (Test-Path -LiteralPath $LegacyPath -PathType Leaf) {
            Remove-Item -LiteralPath $LegacyPath -Force
        }
    }
}

Push-Location $ProjectRoot
try {
    $QMakeStash = Join-Path $ProjectRoot ".qmake.stash"
    if (Test-Path -LiteralPath $QMakeStash) {
        Remove-Item -LiteralPath $QMakeStash -Force
    }

    $Cmd = "call `"$VsDevCmd`" -arch=x64 && `"$QMake`" mergeholo.pro -spec win32-msvc $QMakeConfig"
    if ($Clean) {
        $Cmd += " && if exist $Makefile nmake /f $Makefile clean"
    }
    $Cmd += " && nmake /f $Makefile"
    cmd.exe /s /c $Cmd
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE"
    }

    $imcRuntime = Join-Path $repoRoot "vendor\imc60g\bin\x64\IMC_Library_x64.dll"
    if (-not (Test-Path -LiteralPath $imcRuntime)) { throw "Missing IMC runtime: $imcRuntime" }
    Copy-Item -LiteralPath $imcRuntime -Destination (Join-Path $outputDir "IMC_Library_x64.dll") -Force
    & powershell -ExecutionPolicy Bypass -File (Join-Path $repoRoot "scripts\verify_imc60g_sdk.ps1") `
        -RepoRoot $repoRoot -RuntimeDirectory $outputDir
    if ($LASTEXITCODE -ne 0) { throw "IMC60G SDK verification failed" }

    if (-not $SkipDeploy.IsPresent) {
        Write-Host "Deploying runtime dependencies to $OutDir"
        if (-not (Test-Path -LiteralPath $OutDir)) {
            New-Item -ItemType Directory -Path $OutDir | Out-Null
        }
        Deploy-HoloPipelineRuntime -DestinationRoot $OutDir
        Deploy-CameraRuntime -DestinationRoot $OutDir
        Deploy-MsvcRuntime -DestinationRoot $OutDir -VsDevCmdPath $VsDevCmd
        Deploy-MergeHoloConfig -DestinationRoot $OutDir
        Remove-ObsoletePrintRuntime -DestinationRoot $OutDir
    }
}
finally {
    Pop-Location
}

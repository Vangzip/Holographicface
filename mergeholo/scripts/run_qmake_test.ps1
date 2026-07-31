param([Parameter(Mandatory = $true)][string]$Project)
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$projectPath = [IO.Path]::GetFullPath((Join-Path $repoRoot $Project))
if (-not (Test-Path -LiteralPath $projectPath)) { throw "Missing qmake project: $projectPath" }
$targetLine = Select-String -LiteralPath $projectPath -Pattern '^\s*TARGET\s*=\s*(\S+)' | Select-Object -First 1
if (-not $targetLine) { throw "TARGET is missing from $projectPath" }
$target = $targetLine.Matches[0].Groups[1].Value
$buildDir = Join-Path $repoRoot ("FF-tmp\qmake-tests\" + $target)
New-Item -ItemType Directory -Force $buildDir | Out-Null
$vs = "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
$qmake = "C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe"
$previousPath = $env:Path
Push-Location $buildDir
try {
    $command = "call `"$vs`" -arch=x64 -host_arch=x64 >nul && " +
        "`"$qmake`" `"$projectPath`" `"CONFIG+=release`" -o Makefile && " +
        "nmake /NOLOGO /F Makefile.Release clean && " +
        "nmake /NOLOGO /F Makefile.Release"
    cmd /d /c $command
    if ($LASTEXITCODE -ne 0) { throw "qmake test build failed: $Project" }
    $exe = Join-Path $buildDir "release\$target.exe"
    if (-not (Test-Path -LiteralPath $exe)) { throw "Missing test executable: $exe" }
    $env:QT_QPA_PLATFORM = "offscreen"
    $env:Path = "$(Split-Path -Parent $qmake);$previousPath"
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "qmake test failed: $target" }
}
finally {
    $env:Path = $previousPath
    Pop-Location
}

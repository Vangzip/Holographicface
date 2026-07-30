param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$RuntimeDirectory = ""
)
$ErrorActionPreference = "Stop"
$header = Join-Path $RepoRoot "vendor\imc60g\include\IMC_Library.h"
$errors = Join-Path $RepoRoot "vendor\imc60g\include\errorcode.h"
$library = Join-Path $RepoRoot "vendor\imc60g\lib\x64\IMC_Library_x64.lib"
$runtime = Join-Path $RepoRoot "vendor\imc60g\bin\x64\IMC_Library_x64.dll"
@($header, $errors, $library, $runtime) | ForEach-Object {
    if (-not (Test-Path -LiteralPath $_)) { throw "Missing IMC60G SDK artifact: $_" }
}
$bytes = [IO.File]::ReadAllBytes($runtime)
$peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
$machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
if ($machine -ne 0x8664) { throw "IMC runtime is not x64: machine=0x$($machine.ToString('X4'))" }
$required = @(
    "IMC_CloseCard","IMC_ClrAxSts","IMC_DelEcatComm","IMC_GetAxEncPos32",
    "IMC_GetAxPrfPos32","IMC_GetAxStopReason","IMC_GetAxSts","IMC_GetCardsNum",
    "IMC_GetEcatAxErrCode","IMC_GetEcatAxSdo","IMC_GetEcatErrCode","IMC_GetEcatMasterInfo",
    "IMC_GetEcatMasterSts","IMC_GetEmgSts","IMC_GetEmgTrigLevelInv",
    "IMC_InitEcatComm","IMC_JogPrf","IMC_OpenCard","IMC_OpenCardEx",
    "IMC_ScanCardEcat","IMC_ServoOff","IMC_ServoOn","IMC_SetAxCurPos",
    "IMC_SetAxEndVel","IMC_SetAxMvPara","IMC_SetAxStopDec","IMC_SetEcatAxSdo",
    "IMC_SetEmgTrigLevelInv","IMC_StartEcatComm","IMC_StartJogMove",
    "IMC_StartPtpMove","IMC_StopMove","IMC_SyncAxPos"
)
$vs = "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
$exports = cmd /d /c "call `"$vs`" -arch=x64 -host_arch=x64 >nul && dumpbin /nologo /exports `"$runtime`""
foreach ($name in $required) {
    if (-not ($exports | Select-String -SimpleMatch $name)) { throw "Missing x64 IMC export: $name" }
}
if ($RuntimeDirectory) {
    $staged = Join-Path $RuntimeDirectory "IMC_Library_x64.dll"
    if (-not (Test-Path -LiteralPath $staged)) { throw "Runtime DLL was not staged: $staged" }
    if ((Get-FileHash $staged).Hash -ne (Get-FileHash $runtime).Hash) {
        throw "Staged IMC runtime hash differs from vendored runtime"
    }
}
Write-Output "IMC60G x64 SDK verification passed."

param(
    [ValidateSet("all", "depth", "mesh", "model", "multiview", "elemental")]
    [string]$Stage = "all",
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Exe = Join-Path $ProjectRoot "00-bin\mergeholo.exe"
$Config = Join-Path $ProjectRoot "config\holo_config.microtest.ini"

if (-not (Test-Path -LiteralPath $Exe)) {
    throw "mergeholo.exe not found: $Exe. Build first with scripts\build.ps1."
}

$RunArgs = @("--pipeline", "--config", $Config, "--stage", $Stage)
if ($DryRun) {
    $RunArgs += "--dry-run"
}

& $Exe @RunArgs
exit $LASTEXITCODE

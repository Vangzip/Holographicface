param(
    [string]$QtRoot = "",
    [string]$VsDevCmd = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$outputDir = Join-Path $repoRoot "00-bin"

function Invoke-CheckedPowerShell {
    param(
        [Parameter(Mandatory = $true)][string]$Script,
        [string[]]$Arguments = @()
    )
    & powershell -NoProfile -ExecutionPolicy Bypass -File $Script @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed: $Script $($Arguments -join ' ')"
    }
}

function Assert-PeX64 {
    param([Parameter(Mandatory = $true)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing x64 binary: $Path"
    }
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 64) { throw "Invalid PE binary: $Path" }
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
    $machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
    if ($machine -ne 0x8664) {
        throw "Binary is not x64: $Path machine=0x$($machine.ToString('X4'))"
    }
}

function New-BmpString {
    param([Parameter(Mandatory = $true)][int[]]$CodePoints)
    return -join ($CodePoints | ForEach-Object { [char]$_ })
}

function Find-ByteSequence {
    param(
        [Parameter(Mandatory = $true)][byte[]]$Bytes,
        [Parameter(Mandatory = $true)][byte[]]$Needle
    )
    for ($offset = 0; $offset -le $Bytes.Length - $Needle.Length; ++$offset) {
        $matches = $true
        for ($index = 0; $index -lt $Needle.Length; ++$index) {
            if ($Bytes[$offset + $index] -ne $Needle[$index]) {
                $matches = $false
                break
            }
        }
        if ($matches) { return $offset }
    }
    return -1
}

function Assert-Utf16BinaryText {
    param(
        [Parameter(Mandatory = $true)][byte[]]$Bytes,
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Label,
        [switch]$Absent
    )
    $offset = Find-ByteSequence -Bytes $Bytes -Needle ([Text.Encoding]::Unicode.GetBytes($Text))
    if ($Absent.IsPresent -and $offset -ge 0) {
        throw "Unexpected mojibake UTF-16 text in production binary: $Label offset=$offset"
    }
    if (-not $Absent.IsPresent -and $offset -lt 0) {
        throw "Missing expected UTF-16 UI text in production binary: $Label"
    }
}

function Read-IniValues {
    param([Parameter(Mandatory = $true)][string]$Path)
    $values = @{}
    $section = ""
    foreach ($rawLine in Get-Content -LiteralPath $Path) {
        $line = $rawLine.Trim()
        if (-not $line -or $line.StartsWith("#") -or $line.StartsWith(";")) { continue }
        if ($line -match '^\[([^]]+)\]$') {
            $section = $Matches[1].ToLowerInvariant()
            continue
        }
        $separator = $line.IndexOf('=')
        if ($separator -lt 1 -or -not $section) { continue }
        $key = $line.Substring(0, $separator).Trim().ToLowerInvariant()
        $value = $line.Substring($separator + 1).Trim()
        $values["$section.$key"] = $value
    }
    return $values
}

function Assert-IniValue {
    param(
        [Parameter(Mandatory = $true)][hashtable]$Values,
        [Parameter(Mandatory = $true)][string]$Key,
        [Parameter(Mandatory = $true)][string]$Expected
    )
    if (-not $Values.ContainsKey($Key) -or $Values[$Key] -ne $Expected) {
        throw "Unexpected INI value $Key='$($Values[$Key])'; expected '$Expected'"
    }
}

Invoke-CheckedPowerShell -Script (Join-Path $PSScriptRoot "verify_imc60g_sdk.ps1")

$projects = @(
    "printing/tests/print_config_tests.pro",
    "printing/tests/imc60g_motion_tests.pro",
    "printing/tests/sv660n_exposure_tests.pro",
    "printing/tests/v2_print_timing_tests.pro",
    "printing/tests/v2_presenter_contract_tests.pro",
    "printing/tests/v2_print_engine_tests.pro",
    "widgets/tests/print9030_dialog_tests.pro"
)
foreach ($project in $projects) {
    Invoke-CheckedPowerShell -Script (Join-Path $PSScriptRoot "run_qmake_test.ps1") `
        -Arguments @("-Project", $project)
    Write-Host "$([IO.Path]::GetFileNameWithoutExtension($project)): PASS"
}

$buildArgs = @("-Config", "release")
if ($QtRoot) { $buildArgs += @("-QtRoot", $QtRoot) }
if ($VsDevCmd) { $buildArgs += @("-VsDevCmd", $VsDevCmd) }
Invoke-CheckedPowerShell -Script (Join-Path $PSScriptRoot "build.ps1") -Arguments $buildArgs

$exe = Join-Path $outputDir "mergeholo.exe"
$imcDll = Join-Path $outputDir "IMC_Library_x64.dll"
Assert-PeX64 -Path $exe
Assert-PeX64 -Path $imcDll
$exeBytes = [IO.File]::ReadAllBytes($exe)
$memoryText = New-BmpString -CodePoints @(0x5185, 0x5B58)
$sheetText = New-BmpString -CodePoints @(0x5F20)
$unrecognizedText = New-BmpString -CodePoints @(0x672A, 0x8BC6, 0x522B, 0x7684)
$errorText = New-BmpString -CodePoints @(0x9519, 0x8BEF)
$mojibakeMemoryText = New-BmpString -CodePoints @(0x9350, 0x546D, 0x74E8)
Assert-Utf16BinaryText -Bytes $exeBytes `
    -Text "$memoryText elemental: %1 $sheetText" -Label "memory elemental source summary"
Assert-Utf16BinaryText -Bytes $exeBytes `
    -Text "UNRECOGNIZED_IMC_ERROR: $unrecognizedText IMC errorcode.h $errorText" `
    -Label "IMC unknown-error diagnostic"
Assert-Utf16BinaryText -Bytes $exeBytes `
    -Text "$mojibakeMemoryText elemental" -Label "legacy GBK-decoded memory summary" -Absent
Invoke-CheckedPowerShell -Script (Join-Path $PSScriptRoot "verify_imc60g_sdk.ps1") `
    -Arguments @("-RepoRoot", $repoRoot, "-RuntimeDirectory", $outputDir)

foreach ($relativePath in @("config/imc60g_print.ini", "config/print_9030.ini")) {
    $source = Join-Path $repoRoot $relativePath
    $staged = Join-Path $outputDir $relativePath
    $sourceExists = Test-Path -LiteralPath $source -PathType Leaf
    $stagedExists = Test-Path -LiteralPath $staged -PathType Leaf
    if (-not $sourceExists -or -not $stagedExists) {
        throw "Missing staged print configuration: $staged"
    }
    if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne
        (Get-FileHash -LiteralPath $staged -Algorithm SHA256).Hash) {
        throw "Staged print configuration differs from source: $relativePath"
    }
}

$printValues = Read-IniValues -Path (Join-Path $repoRoot "config/print_9030.ini")
$approvedPrintValues = @{
    "meta.version" = "2";
    "main.row_spacing_mm" = "0.5"; "main.column_spacing_mm" = "0.5";
    "main.grid_rows" = "150"; "main.grid_columns" = "150";
    "main.width_scale" = "3.8"; "main.height_scale" = "2.8";
    "main.add_temp_pulse" = "16000"; "main.lead_pulse" = "1000";
    "axis_x.subdivision" = "40"; "axis_x.resolution" = "50";
    "axis_x.speed_of_movement" = "5000"; "axis_x.accelerated_velocity" = "50000";
    "axis_x.start_speed" = "500"; "axis_x.stop_speed" = "500";
    "axis_x.max_distance" = "150"; "axis_x.change_direction" = "false";
    "axis_x.electrical_status" = "false";
    "axis_y.subdivision" = "40"; "axis_y.resolution" = "50";
    "axis_y.speed_of_movement" = "60000"; "axis_y.accelerated_velocity" = "150000";
    "axis_y.start_speed" = "0"; "axis_y.stop_speed" = "0";
    "axis_y.max_distance" = "150"; "axis_y.change_direction" = "false";
    "axis_y.electrical_status" = "false"
}
foreach ($entry in $approvedPrintValues.GetEnumerator()) {
    Assert-IniValue -Values $printValues -Key $entry.Key -Expected $entry.Value
}
$profileValues = Read-IniValues -Path (Join-Path $repoRoot "config/imc60g_print.ini")
Assert-IniValue -Values $profileValues -Key "print.step_pulse" -Expected "1000"
$calculatedStep = [int]$printValues["axis_y.subdivision"] *
    [int]$printValues["axis_y.resolution"] *
    [double]::Parse($printValues["main.column_spacing_mm"],
        [Globalization.CultureInfo]::InvariantCulture)
if ($calculatedStep -ne [int]$profileValues["print.step_pulse"]) {
    throw "Active V2 Y pulse basis mismatch: calculated=$calculatedStep expected=$($profileValues['print.step_pulse'])"
}

foreach ($legacyDll in @(
    "DfjzhControlerDll.dll", "dfjzh6052dll.dll", "dfjzh6052dll0.dll", "CH365DLL.DLL"
)) {
    $legacyPath = Join-Path $outputDir $legacyDll
    if (Test-Path -LiteralPath $legacyPath) {
        throw "Obsolete print runtime must not be deployed: $legacyPath"
    }
}

$previousPlatform = $env:QT_QPA_PLATFORM
$startupLog = Join-Path $repoRoot "runs/latest/imc60g_startup.log"
$startupLogLength = if (Test-Path -LiteralPath $startupLog) {
    (Get-Item -LiteralPath $startupLog).Length
} else { 0 }
try {
    $env:QT_QPA_PLATFORM = "offscreen"
    $startupOutput = & $exe --mergeholo-help 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) { throw "mergeholo startup diagnostics probe failed" }
}
finally {
    $env:QT_QPA_PLATFORM = $previousPlatform
}
$newStartupLogLength = if (Test-Path -LiteralPath $startupLog) {
    (Get-Item -LiteralPath $startupLog).Length
} else { 0 }
if ($newStartupLogLength -le $startupLogLength) {
    throw "Startup diagnostics were not appended to: $startupLog"
}
$startupBytes = [IO.File]::ReadAllBytes($startupLog)
$newStartupBytes = $startupBytes[$startupLogLength..($startupBytes.Length - 1)]
$startupDiagnostics = [Text.Encoding]::UTF8.GetString($newStartupBytes)
$expectedRuntimeHash = (Get-FileHash -LiteralPath $imcDll -Algorithm SHA256).Hash.ToLowerInvariant()
foreach ($requiredText in @(
    "IMC60G startup architecture=x64",
    "IMC60G runtime path=$([IO.Path]::GetFullPath($imcDll))",
    "IMC60G runtime sha256=$expectedRuntimeHash",
    "IMC60G profile version=1 card=0 axis_x=1 axis_y=0 home_order=Y,X",
    "IMC60G exposure backend=SV660N internal position compare DO1",
    "IMC60G startup action=diagnostics-only; card remains closed",
    "IMC60G startup log path="
)) {
    if (-not $startupDiagnostics.Contains($requiredText)) {
        throw "Missing startup diagnostic '$requiredText'. Log:`n$startupDiagnostics"
    }
}

Write-Host "mergeholo x64 build: PASS"
Write-Host "runtime staging: PASS"

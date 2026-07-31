<#
.SYNOPSIS
Runs exactly one guarded IMC60G acceptance stage and writes an evidence record.

.DESCRIPTION
This script never calls the IMC SDK, never sends Connect/Home/Motion/DO1
commands, and never advances to another stage.  With -LaunchUi it only starts
the normal mergeholo UI; every hardware action remains an explicit operator UI
action.  The discover stage cannot prove card/EtherCAT state without a missing
independent discovery interface, so it is recorded as blocked rather than
opening the card or fabricating a pass.

.PARAMETER Stage
The one acceptance stage to prepare and record.

.PARAMETER OperatorConfirmedSafe
Required on every invocation. Confirms the physical E-stop was function-tested,
negative-limit wiring/direction/mechanical endpoints were checked, X/Y travel
is clear, DO1 uses an isolated safe load, and the second display cannot affect
other safety systems for this run.

.PARAMETER LaunchUi
Starts mergeholo.exe --ui after deployment checks. Opening the UI must not open
the card. The script never clicks or invokes any hardware control.

.PARAMETER DryRun
Creates a blocked evidence record without launching the UI or prompting. This
is the only mode intended for automated safety testing.

.PARAMETER Result
Optional command-line fail or blocked result. A pass is accepted only when it is
entered interactively after this invocation launches the UI.

.PARAMETER Observations
Operator-entered measurements, log references, and reasons supporting Result.

.PARAMETER EvidenceFiles
Existing stage-specific evidence files such as controller logs, scope captures,
photos, or exported measurements. Every pass needs at least one hashable file.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts\run_imc60g_acceptance.ps1 `
    -Stage discover -OperatorConfirmedSafe -DryRun

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts\run_imc60g_acceptance.ps1 `
    -Stage home -OperatorConfirmedSafe -LaunchUi `
    -EvidenceFiles "D:\imc60g-evidence\home-measurement.csv"
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("discover", "home", "xy-small", "display", "do1", "one-row", "serpentine", "cancel", "end-to-end")]
    [string]$Stage,

    [switch]$OperatorConfirmedSafe,

    [switch]$LaunchUi,
    [switch]$DryRun,

    [ValidateSet("pass", "fail", "blocked")]
    [string]$Result,

    [string]$Observations,
    [string[]]$EvidenceFiles = @(),
    [string]$Operator = [Environment]::UserName
)

$ErrorActionPreference = "Stop"

if (-not $OperatorConfirmedSafe.IsPresent) {
    throw "Use -OperatorConfirmedSafe only after function-testing the physical E-stop; checking negative-limit wiring, direction, and mechanical endpoints; clearing X/Y travel; connecting DO1 only to an isolated safe load; and confirming the second display cannot affect other safety systems."
}
if ($DryRun.IsPresent -and $LaunchUi.IsPresent) {
    throw "-DryRun and -LaunchUi cannot be combined. Dry-run must never start the application."
}
if ($DryRun.IsPresent -and $Result) {
    throw "-DryRun always records blocked; do not provide -Result."
}

$resultProvidedOnCommandLine = $PSBoundParameters.ContainsKey("Result")

function ConvertTo-FlowLine {
    param([AllowNull()][string]$Value)
    if ($null -eq $Value) { return "" }
    return (($Value `
        -replace "[\u0000-\u001F\u007F-\u009F\u200E\u200F\u2028\u2029\u202A-\u202E\u2066-\u2069]", " ").Trim())
}

function Get-ArtifactRecord {
    param([Parameter(Mandatory = $true)][string]$Path)
    $absolute = [IO.Path]::GetFullPath($Path)
    $exists = Test-Path -LiteralPath $absolute -PathType Leaf
    $sha256 = $null
    $lastWriteTimeUtc = $null
    $peMachine = $null
    $isX64 = $null
    if ($exists) {
        $lastWriteTimeUtc = (Get-Item -LiteralPath $absolute).LastWriteTimeUtc.ToString("o")
        $sha256 = (Get-FileHash -LiteralPath $absolute -Algorithm SHA256).Hash
        if ([IO.Path]::GetExtension($absolute).ToLowerInvariant() -in @(".exe", ".dll")) {
            try {
                $bytes = [IO.File]::ReadAllBytes($absolute)
                if ($bytes.Length -lt 64) { throw "PE file is too short." }
                $peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
                $machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
                $peMachine = "0x$($machine.ToString('X4'))"
                $isX64 = ($machine -eq 0x8664)
            } catch {
                $peMachine = "invalid"
                $isX64 = $false
            }
        }
    }
    [ordered]@{
        path = $absolute
        exists = $exists
        sha256 = $sha256
        last_write_time_utc = $lastWriteTimeUtc
        pe_machine = $peMachine
        is_x64 = $isX64
    }
}

function Read-FlatIni {
    param([Parameter(Mandatory = $true)][string]$Path)
    $values = [ordered]@{}
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $values
    }
    $section = ""
    foreach ($rawLine in Get-Content -LiteralPath $Path) {
        $line = $rawLine.Trim()
        if (-not $line -or $line.StartsWith("#") -or $line.StartsWith(";")) {
            continue
        }
        if ($line.StartsWith("[") -and $line.EndsWith("]")) {
            $section = $line.Substring(1, $line.Length - 2).Trim()
            continue
        }
        $separator = $line.IndexOf("=")
        if ($separator -lt 1) {
            continue
        }
        $key = $line.Substring(0, $separator).Trim()
        $value = $line.Substring($separator + 1).Trim()
        $flatKey = if ($section) { "$section.$key" } else { $key }
        $values[$flatKey] = $value
    }
    return $values
}

function Add-FlowLog {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Event,
        [Parameter(Mandatory = $true)][string]$Detail
    )
    $stamp = (Get-Date).ToUniversalTime().ToString("o")
    $safeEvent = ConvertTo-FlowLine $Event
    $safeDetail = ConvertTo-FlowLine $Detail
    Add-Content -LiteralPath $Path -Encoding UTF8 -Value "$stamp stage=$Stage event=$safeEvent $safeDetail"
}

function Require-ExactProfile {
    param(
        [Parameter(Mandatory = $true)][System.Collections.IDictionary]$Values,
        [Parameter(Mandatory = $true)][string]$Key,
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Problems
    )
    if (-not $Values.Contains($Key) -or [string]$Values[$Key] -ne $Expected) {
        $actual = if ($Values.Contains($Key)) { [string]$Values[$Key] } else { "<missing>" }
        $Problems.Add("$Key expected '$Expected' but found '$actual'.")
    }
}

function Find-LatestPassedStageEvidence {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$RequiredStage,
        [Parameter(Mandatory = $true)][string]$ExecutableSha256,
        [Parameter(Mandatory = $true)][string]$RuntimeSha256,
        [Parameter(Mandatory = $true)][string]$HardwareConfigSha256,
        [Parameter(Mandatory = $true)][string]$PrintConfigSha256
    )
    if (-not (Test-Path -LiteralPath $Root -PathType Container)) { return $null }
    foreach ($directory in Get-ChildItem -LiteralPath $Root -Directory | Sort-Object LastWriteTimeUtc -Descending) {
        $candidate = Join-Path $directory.FullName "acceptance.json"
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) { continue }
        try {
            $record = Get-Content -LiteralPath $candidate -Raw | ConvertFrom-Json
        } catch {
            continue
        }
        if ($record.stage -ne $RequiredStage -or $record.result -ne "pass" -or
            $record.pass -ne $true -or $record.single_stage_only -ne $true -or
            $record.safety_gate.operator_confirmed_safe -ne $true -or
            $record.script_actions.ui_launched -ne $true -or
            $record.pass_gate.ui_required_and_launched -ne $true -or
            $record.pass_gate.operator_attestation_confirmed -ne $true -or
            $record.pass_gate.result_provided_on_command_line -ne $false -or
            $record.pass_gate.observations_entered_after_stage -ne $true -or
            @($record.pass_gate.problems).Count -ne 0 -or
            @($record.deployment_gate_problems).Count -ne 0 -or
            @($record.supporting_evidence).Count -lt 1) {
            continue
        }
        if ($record.artifacts.executable.sha256 -ne $ExecutableSha256 -or
            $record.artifacts.imc_runtime_dll.sha256 -ne $RuntimeSha256 -or
            $record.artifacts.deployed_hardware_config.sha256 -ne $HardwareConfigSha256 -or
            $record.artifacts.deployed_print_config.sha256 -ne $PrintConfigSha256) {
            continue
        }
        $supportingEvidenceValid = $true
        foreach ($supporting in @($record.supporting_evidence)) {
            if ([string]::IsNullOrWhiteSpace([string]$supporting.path)) {
                $supportingEvidenceValid = $false
                break
            }
            $currentSupporting = Get-ArtifactRecord ([string]$supporting.path)
            if (-not $currentSupporting.exists -or
                $currentSupporting.sha256 -ne [string]$supporting.sha256) {
                $supportingEvidenceValid = $false
                break
            }
        }
        if (-not $supportingEvidenceValid) { continue }
        return Get-ArtifactRecord $candidate
    }
    return $null
}

$repoRoot = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$startedUtc = (Get-Date).ToUniversalTime()
$timestamp = $startedUtc.ToString("yyyyMMdd-HHmmss-fff")
$evidenceRoot = Join-Path $repoRoot "runs\imc60g-acceptance"
$evidenceDirectory = Join-Path $evidenceRoot $timestamp
New-Item -ItemType Directory -Path $evidenceDirectory -Force | Out-Null

$jsonPath = Join-Path $evidenceDirectory "acceptance.json"
$flowLogPath = Join-Path $evidenceDirectory "print_flow.log"
New-Item -ItemType File -Path $flowLogPath -Force | Out-Null

$exe = Get-ArtifactRecord (Join-Path $repoRoot "00-bin\mergeholo.exe")
$dll = Get-ArtifactRecord (Join-Path $repoRoot "00-bin\IMC_Library_x64.dll")
$vendoredDll = Get-ArtifactRecord (Join-Path $repoRoot "vendor\imc60g\bin\x64\IMC_Library_x64.dll")
$hardwareConfig = Get-ArtifactRecord (Join-Path $repoRoot "00-bin\config\imc60g_print.ini")
$printConfig = Get-ArtifactRecord (Join-Path $repoRoot "00-bin\config\print_9030.ini")
$sourceHardwareConfig = Get-ArtifactRecord (Join-Path $repoRoot "config\imc60g_print.ini")
$sourcePrintConfig = Get-ArtifactRecord (Join-Path $repoRoot "config\print_9030.ini")
$startupLogPath = [IO.Path]::GetFullPath((Join-Path $repoRoot "runs\latest\imc60g_startup.log"))

$profileValues = Read-FlatIni $hardwareConfig.path
$printValues = Read-FlatIni $printConfig.path
$problems = [System.Collections.Generic.List[string]]::new()

foreach ($artifact in @(
    $exe, $dll, $vendoredDll, $hardwareConfig, $printConfig,
    $sourceHardwareConfig, $sourcePrintConfig
)) {
    if (-not $artifact.exists) {
        $problems.Add("Required deployed artifact is missing: $($artifact.path)")
    }
}

foreach ($binary in @($exe, $dll, $vendoredDll)) {
    if ($binary.exists -and $binary.is_x64 -ne $true) {
        $problems.Add("Required binary is not x64 PE machine 0x8664: $($binary.path) machine=$($binary.pe_machine)")
    }
}

foreach ($pair in @(
    [pscustomobject]@{ Deployed = $dll; Source = $vendoredDll; Label = "IMC runtime DLL" },
    [pscustomobject]@{ Deployed = $hardwareConfig; Source = $sourceHardwareConfig; Label = "IMC hardware profile" },
    [pscustomobject]@{ Deployed = $printConfig; Source = $sourcePrintConfig; Label = "print configuration" }
)) {
    if ($pair.Deployed.exists -and $pair.Source.exists -and
        $pair.Deployed.sha256 -ne $pair.Source.sha256) {
        $problems.Add("Deployed $($pair.Label) differs from its verified source.")
    }
}

if ($hardwareConfig.exists) {
    $approvedHardwareProfile = [ordered]@{
        "profile.version" = "1"; "profile.card_index" = "0";
        "profile.axis_x" = "1"; "profile.axis_y" = "0"; "profile.home_order" = "Y,X";
        "homing.direction_x" = "-1"; "homing.direction_y" = "-1";
        "homing.speed" = "32000"; "homing.acceleration" = "80000";
        "homing.deceleration" = "80000"; "homing.timeout_ms" = "180000";
        "homing.stable_ms" = "500"; "homing.minimum_move_pulse" = "100";
        "homing.backoff_x_pulse" = "28000"; "homing.backoff_y_pulse" = "92000";
        "homing.backoff_speed" = "10000"; "homing.backoff_timeout_ms" = "30000";
        "print.step_pulse" = "1000"; "print.forward_delay_pulse" = "4000";
        "print.reverse_fixed_pulse" = "2000"; "print.default_add_temp_pulse" = "16000";
        "print.default_lead_pulse" = "1000"; "print.exposure_offset_pulse" = "2000";
        "sv660n.do_function" = "25"; "sv660n.point_index" = "1";
        "sv660n.mode" = "0"; "sv660n.width" = "1000";
        "sv660n.use_user_unit" = "true"; "sv660n.positive_attribute" = "129";
        "sv660n.negative_attribute" = "130"
    }
    foreach ($entry in $approvedHardwareProfile.GetEnumerator()) {
        Require-ExactProfile $profileValues $entry.Key $entry.Value $problems
    }
}

if ($printConfig.exists) {
    $approvedPrintConfig = [ordered]@{
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
    foreach ($entry in $approvedPrintConfig.GetEnumerator()) {
        Require-ExactProfile $printValues $entry.Key $entry.Value $problems
    }
}

$stagePrerequisites = [ordered]@{
    "discover" = @()
    "home" = @()
    "xy-small" = @("home")
    "display" = @()
    "do1" = @("home", "xy-small")
    "one-row" = @("home", "xy-small", "display", "do1")
    "serpentine" = @("one-row")
    "cancel" = @("serpentine")
    "end-to-end" = @("home", "xy-small", "display", "do1", "one-row", "serpentine", "cancel")
}
$prerequisiteEvidence = [ordered]@{}
if ($exe.exists -and $dll.exists -and $hardwareConfig.exists -and $printConfig.exists) {
    foreach ($requiredStage in $stagePrerequisites[$Stage]) {
        $evidence = Find-LatestPassedStageEvidence `
            -Root $evidenceRoot `
            -RequiredStage $requiredStage `
            -ExecutableSha256 $exe.sha256 `
            -RuntimeSha256 $dll.sha256 `
            -HardwareConfigSha256 $hardwareConfig.sha256 `
            -PrintConfigSha256 $printConfig.sha256
        if ($null -eq $evidence) {
            $problems.Add("Missing compatible passed prerequisite evidence for stage '$requiredStage'.")
        } else {
            $prerequisiteEvidence[$requiredStage] = $evidence
        }
    }
}

$stageInstructions = [ordered]@{
    "discover" = @(
        "Do not click Connect and Home and do not use any vendor command that opens the card.",
        "Verify only deployed x64 executable, DLL hashes, configuration, startup log, and that opening the dialog causes no hardware action.",
        "Card count and EtherCAT OP cannot be verified by this script without a separate read-only interface; record this stage blocked."
    )
    "home" = @(
        "With E-stop reachable and travel clear, click Connect and Home exactly once in the UI.",
        "Record Y(Axis0) negative-limit hit, 92000-pulse backoff and zero before X(Axis1) negative-limit hit, 28000-pulse backoff and zero.",
        "Record EtherCAT OP and both Servo states; fail immediately on unexpected direction, alarm, or limit behavior."
    )
    "xy-small" = @(
        "Enter the reviewed low speed and small distance in the UI; do not use production speed for the first move.",
        "Exercise X-/X+/Y-/Y+, the Stop button, measured direction/displacement, stopped state, and return position.",
        "Press physical E-stop on any unexpected direction or distance and record fail."
    )
    "display" = @(
        "Keep motion disconnected or stopped. Present numbered frames only on the selected non-primary display.",
        "Record output identity, refresh rate, visible frame order, Present results and physical VBlank statistics.",
        "No motion or DO1 operation belongs to this stage."
    )
    "do1" = @(
        "Connect an oscilloscope, isolated input, or approved safe load to SV660N DO1+/DO1-; never connect a hazardous production load for first validation.",
        "At reviewed low speed, validate one positive and one negative Y crossing.",
        "Record H04/H18/H19 SDO writes/readbacks, target positions, measured polarity, width, trigger position, and compare-enable-off after each run."
    )
    "one-row" = @(
        "Use a reviewed low-risk one-row image set with exact rows*columns count.",
        "Record forward frame order, Y target, VBlank pacing, DO1 trigger and terminal cleanup/return-to-zero evidence."
    )
    "serpentine" = @(
        "Use a reviewed small two-row job only after one-row passes.",
        "Record forward then reverse frame order, X row step, both Y targets, VBlank pacing, DO1 triggers and return behavior."
    )
    "cancel" = @(
        "Verify pause occurs only after the row boundary with exposure disabled and axes stopped.",
        "Verify resume rechecks hardware and does not duplicate completed frames.",
        "Verify cancel and one controlled failure stop axes, disarm DO1, do not start the next motion, aggregate errors and report the final safe/fault state."
    )
    "end-to-end" = @(
        "Use the same reviewed small job first from elemental memory and then from a folder source.",
        "Compare image count/order, motion and DO1 logs, output timing, cleanup and return results between both sources."
    )
}

$operatorRecord = ConvertTo-FlowLine $Operator
if ([string]::IsNullOrWhiteSpace($operatorRecord)) { $operatorRecord = "unknown" }
if ($operatorRecord.Length -gt 200) { $operatorRecord = $operatorRecord.Substring(0, 200) }

Add-FlowLog $flowLogPath "stage_begin" "operator=$operatorRecord safety_confirmed=true dry_run=$($DryRun.IsPresent) launch_ui=$($LaunchUi.IsPresent)"
foreach ($instruction in $stageInstructions[$Stage]) {
    Write-Host "- $instruction"
    Add-FlowLog $flowLogPath "instruction" ($instruction -replace "`r|`n", " ")
}

$uiProcessId = $null
$uiLaunched = $false
if ($DryRun.IsPresent) {
    $Result = "blocked"
    $Observations = "Safety dry-run only: no application, SDK, card, EtherCAT, Servo, motion, display Present, or DO1 command was started."
} elseif ($problems.Count -gt 0) {
    $Result = "blocked"
    $Observations = "Deployment/profile gate failed: " + ($problems -join " ")
} elseif ($Stage -eq "discover") {
    if ($LaunchUi.IsPresent) {
        $process = Start-Process -FilePath $exe.path -ArgumentList @("--ui") -PassThru
        $uiProcessId = $process.Id
        $uiLaunched = $true
        Add-FlowLog $flowLogPath "ui_started" "pid=$uiProcessId args=--ui; script sent no hardware command"
    }
    $Result = "blocked"
    if (-not $Observations) {
        $Observations = Read-Host "Record deployment/startup observations; do not click Connect and Home"
    }
    $Observations = "No independent read-only card/EtherCAT discovery interface exists; hardware discovery is unverified and this stage is blocked. " + $Observations
} else {
    if ($LaunchUi.IsPresent) {
        $process = Start-Process -FilePath $exe.path -ArgumentList @("--ui") -PassThru
        $uiProcessId = $process.Id
        $uiLaunched = $true
        Add-FlowLog $flowLogPath "ui_started" "pid=$uiProcessId args=--ui; all hardware actions are operator-controlled"
        Write-Host "The UI was started. Perform only stage '$Stage'; the script will not advance."
    } else {
        Write-Host "The UI was not started. If needed, start '$($exe.path) --ui' manually and perform only stage '$Stage'."
    }

    while (-not $Result) {
        $entered = (Read-Host "Enter this stage result exactly: pass, fail, or blocked").Trim().ToLowerInvariant()
        if ($entered -in @("pass", "fail", "blocked")) {
            $Result = $entered
        } else {
            Write-Warning "Result must be pass, fail, or blocked."
        }
    }
    if ($Result -eq "pass" -and $resultProvidedOnCommandLine -and -not $Observations) {
        $Observations = "Command-line pass was rejected before observations were collected."
    } elseif (-not $Observations) {
        $Observations = Read-Host "Enter measured observations and evidence/log references"
    }
    if ([string]::IsNullOrWhiteSpace($Observations)) {
        $Result = "blocked"
        $Observations = "Operator did not provide observations; a pass/fail result cannot be substantiated."
    }
}

$supportingEvidence = @()
$passAttestationConfirmed = $false
$evidenceCheckedUtc = $null
$passGateProblems = [System.Collections.Generic.List[string]]::new()
foreach ($evidencePath in $EvidenceFiles) {
    if ([string]::IsNullOrWhiteSpace($evidencePath)) { continue }
    $artifact = Get-ArtifactRecord $evidencePath
    if ($artifact.exists) {
        $supportingEvidence += $artifact
    } else {
        $passGateProblems.Add("Supporting evidence file is missing: $($artifact.path)")
    }
}

if ($Result -eq "pass") {
    $evidenceCheckedUtc = (Get-Date).ToUniversalTime()
    if ($resultProvidedOnCommandLine) {
        $passGateProblems.Add("A pass cannot be supplied with -Result; enter pass interactively after performing the launched UI stage.")
    }
    if (-not $LaunchUi.IsPresent -or -not $uiLaunched) {
        $passGateProblems.Add("A pass requires this invocation to start the UI with -LaunchUi.")
    }
    if ($PSBoundParameters.ContainsKey("Observations")) {
        $passGateProblems.Add("Pass observations must be entered interactively after the stage, not supplied on the command line.")
    }
    if ([string]::IsNullOrWhiteSpace($Observations) -or $Observations.Trim().Length -lt 40) {
        $passGateProblems.Add("A pass requires at least 40 characters of measured observations and log references.")
    }
    if ($supportingEvidence.Count -lt 1) {
        $passGateProblems.Add("A pass requires at least one existing stage-specific file supplied with -EvidenceFiles.")
    }
    foreach ($artifact in $supportingEvidence) {
        $lastWrite = [DateTime]::Parse($artifact.last_write_time_utc).ToUniversalTime()
        if ($lastWrite -lt $startedUtc -or $lastWrite -gt $evidenceCheckedUtc) {
            $passGateProblems.Add("Supporting evidence was not created or updated between this stage's start and evidence check: $($artifact.path)")
        }
    }
    if (-not $resultProvidedOnCommandLine -and $uiLaunched -and $passGateProblems.Count -eq 0) {
        $expectedAttestation = "PASS $Stage VERIFIED"
        $enteredAttestation = Read-Host "Type exactly '$expectedAttestation' to attest the measurements and evidence"
        $passAttestationConfirmed = ($enteredAttestation -ceq $expectedAttestation)
        if (-not $passAttestationConfirmed) {
            $passGateProblems.Add("The exact post-stage operator attestation was not entered.")
        }
    }
    if ($passGateProblems.Count -gt 0) {
        $Result = "blocked"
        $gateDetail = $passGateProblems -join " "
        $Observations = ((ConvertTo-FlowLine $Observations) + " Pass gate rejected: " + $gateDetail).Trim()
        Add-FlowLog $flowLogPath "pass_rejected" $gateDetail
    }
}

$endedUtc = (Get-Date).ToUniversalTime()
$exitCode = switch ($Result) {
    "pass" { 0 }
    "fail" { 2 }
    default { 3 }
}

Add-FlowLog $flowLogPath "stage_end" "result=$Result exit_code=$exitCode observations=$($Observations -replace "`r|`n", " ")"

$record = [ordered]@{
    schema_version = 1
    stage = $Stage
    single_stage_only = $true
    next_stage_automatically_started = $false
    start_time_utc = $startedUtc.ToString("o")
    end_time_utc = $endedUtc.ToString("o")
    duration_seconds = [Math]::Round(($endedUtc - $startedUtc).TotalSeconds, 3)
    exit_code = $exitCode
    result = $Result
    pass = ($Result -eq "pass")
    fail = ($Result -eq "fail")
    blocked = ($Result -eq "blocked")
    operator = $operatorRecord
    observations = $Observations
    safety_gate = [ordered]@{
        operator_confirmed_safe = $true
        confirmation_scope = "Physical E-stop reachable and function-tested; negative-limit wiring, trigger direction, and mechanical endpoints checked; X/Y travel clear; SV660N DO1 connected only to an oscilloscope or isolated approved safe load; second display confirmed unable to affect other safety systems."
        confirmation_time_utc = $startedUtc.ToString("o")
    }
    pass_gate = [ordered]@{
        result_provided_on_command_line = $resultProvidedOnCommandLine
        ui_required_and_launched = ($LaunchUi.IsPresent -and $uiLaunched)
        observations_entered_after_stage = (-not $PSBoundParameters.ContainsKey("Observations"))
        operator_attestation_confirmed = $passAttestationConfirmed
        evidence_checked_time_utc = if ($null -ne $evidenceCheckedUtc) { $evidenceCheckedUtc.ToString("o") } else { $null }
        problems = @($passGateProblems)
    }
    script_actions = [ordered]@{
        sdk_or_pinvoke_used = $false
        connect_or_home_command_sent = $false
        motion_command_sent = $false
        do1_command_sent = $false
        ui_launched = $uiLaunched
        ui_process_id = $uiProcessId
        dry_run = $DryRun.IsPresent
    }
    evidence = [ordered]@{
        directory = [IO.Path]::GetFullPath($evidenceDirectory)
        acceptance_json = [IO.Path]::GetFullPath($jsonPath)
        print_flow_log = [IO.Path]::GetFullPath($flowLogPath)
        startup_log = $startupLogPath
    }
    artifacts = [ordered]@{
        executable = $exe
        imc_runtime_dll = $dll
        vendored_imc_runtime_dll = $vendoredDll
        deployed_hardware_config = $hardwareConfig
        deployed_print_config = $printConfig
        source_hardware_config = $sourceHardwareConfig
        source_print_config = $sourcePrintConfig
    }
    profile = [ordered]@{
        version = $profileValues["profile.version"]
        card_index = $profileValues["profile.card_index"]
        logical_x_physical_axis = $profileValues["profile.axis_x"]
        logical_y_physical_axis = $profileValues["profile.axis_y"]
        home_order = $profileValues["profile.home_order"]
        home_direction_x = $profileValues["homing.direction_x"]
        home_direction_y = $profileValues["homing.direction_y"]
        backoff_x_pulse = $profileValues["homing.backoff_x_pulse"]
        backoff_y_pulse = $profileValues["homing.backoff_y_pulse"]
        print_step_pulse = $profileValues["print.step_pulse"]
        exposure_backend = "SV660N internal position comparison on logical Y / physical Axis0"
        sv660n_do_output = "DO1+/DO1-"
        sv660n_do_function = $profileValues["sv660n.do_function"]
        sv660n_point_index = $profileValues["sv660n.point_index"]
        sv660n_mode = $profileValues["sv660n.mode"]
        sv660n_width = $profileValues["sv660n.width"]
        sv660n_use_user_unit = $profileValues["sv660n.use_user_unit"]
        sv660n_positive_attribute = $profileValues["sv660n.positive_attribute"]
        sv660n_negative_attribute = $profileValues["sv660n.negative_attribute"]
        all_hardware_profile_values = $profileValues
        all_print_config_values = $printValues
    }
    deployment_gate_problems = @($problems)
    prerequisite_evidence = $prerequisiteEvidence
    supporting_evidence = @($supportingEvidence)
    stage_instructions = @($stageInstructions[$Stage])
}

$record | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
Write-Host "Acceptance evidence: $jsonPath"
Write-Host "Flow log: $flowLogPath"
Write-Host "Stage '$Stage' result: $Result (exit code $exitCode)"
exit $exitCode

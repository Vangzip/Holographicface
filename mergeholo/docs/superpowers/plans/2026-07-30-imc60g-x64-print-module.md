# IMC60G x64 Print Module Replacement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the existing DFJZH-oriented printing module with the current V2 IMC60G/SV660N printing behavior, a complete Qt control surface, native x64 SDK integration, and staged connected-hardware verification.

**Architecture:** Keep `mergeholo` as one x64 Qt process. Isolate the vendor API, IMC60G motion, SV660N exposure, V2 timing, D3D/VBlank presentation, job state machine, and UI behind explicit interfaces so every hardware call sequence can be tested with recording fakes before connected-hardware testing.

**Tech Stack:** Qt 5.15.0, C++17, qmake, MSVC 2019 x64, IMC60G x64 SDK, EtherCAT, SV660N SDO position comparison, Direct3D 11, DXGI, PowerShell verification scripts.

## Global Constraints

- Target Windows x64 only; do not add an x86 build, x86 bridge, or legacy DFJZH dependency.
- Use `C:\wzp\Holographicface\按帧送图V2` as the behavioral source of truth.
- Use the SDK from `C:\wzp\Holographicface\按帧送图V2\IMC_Library_Release`.
- Preserve card index 0, logical X to IMC axis 1, logical Y to IMC axis 0.
- Preserve the latest V2 Y-then-X homing order, direction, backoff values, and zeroing behavior.
- Preserve the latest V2 `SV660N_DRIVE_CMP` backend and DO1+/DO1- output.
- Preserve the active V2 `start_new` timing, frame ordering, VBlank, pulse, and cleanup behavior.
- Never report simulated axes or software exposure as successful production hardware.
- Opening the printing dialog must not open the card, release emergency state, enable a Servo, or move an axis.
- Hardware initialization and homing begin only from the explicit **连接并回零** command.
- Any failed preflight blocks exposure and motion.
- Preserve all unrelated user changes in the dirty worktree; stage and commit only files listed by the current task.
- Each task follows red-green-refactor and ends with a focused commit after its tests pass.

---

## File Structure

| File | Responsibility |
| --- | --- |
| `vendor/imc60g/include/IMC_Library.h` | Exact vendor C API declarations. |
| `vendor/imc60g/include/errorcode.h` | Exact vendor return-code declarations. |
| `vendor/imc60g/lib/x64/IMC_Library_x64.lib` | MSVC x64 import library. |
| `vendor/imc60g/bin/x64/IMC_Library_x64.dll` | x64 runtime DLL staged beside `mergeholo.exe`. |
| `Pri/imc60g.pri` | One qmake include path/link/deployment definition for the SDK. |
| `scripts/verify_imc60g_sdk.ps1` | Verifies bitness, hashes, required exports, and staged runtime. |
| `printing/PrintHardwareProfile.{h,cpp}` | Immutable production mapping and exact V2 constants. |
| `config/imc60g_print.ini` | Versioned, reviewable production hardware profile. |
| `printing/IImc60gApi.h` | Mockable subset of IMC functions used by this feature. |
| `printing/Imc60gApi.{h,cpp}` | Native x64 calls and decoded vendor errors. |
| `printing/Imc60gMotionController.{h,cpp}` | Card/EtherCAT/Servo/homing/manual motion lifecycle. |
| `printing/Sv660nExposureController.{h,cpp}` | Exact SV660N SDO arm/disarm behavior. |
| `printing/V2PrintTiming.{h,cpp}` | Pure V2 row, pulse, frame, and exposure calculations. |
| `printing/V2D3DFramePresenter.{h,cpp}` | Latest V2 D3D11/DXGI output and VBlank path. |
| `printing/PrintHardwarePreflight.{h,cpp}` | Fail-closed print readiness checks. |
| `printing/PrintJobRunner.{h,cpp}` | Explicit print state machine and V2 worker orchestration. |
| `printing/PrintController.{h,cpp}` | Serializes connect/home/manual/print commands for the UI. |
| `printing/PrintConfig.{h,cpp}` | Editable, versioned print/motion settings. |
| `printing/tests/test_imc60g_motion.cpp` | Recording-api motion lifecycle tests. |
| `printing/tests/test_sv660n_exposure.cpp` | Exact SDO sequence tests. |
| `printing/tests/test_v2_print_engine.cpp` | Golden timing, sequencing, cleanup, and preflight tests. |
| `widgets/Print9030Dialog.{h,cpp}` | Complete Qt UI behavior and state binding. |
| `ui/Print9030Dialog.ui` | IMC60G production UI layout. |
| `widgets/tests/test_print9030_dialog.cpp` | Button, field, state, persistence, and close tests. |
| `scripts/run_imc60g_acceptance.ps1` | Guarded, stage-by-stage connected-hardware acceptance runner. |
| `docs/imc60g-print-acceptance.md` | Operator checklist and evidence template. |

## Reference Baselines

- V2 motion and exposure adapter:
  `C:\wzp\Holographicface\按帧送图V2\Print\MotionCardAdapter.cpp`
- V2 production constants:
  `C:\wzp\Holographicface\按帧送图V2\Print\MotionCardDefineConfig.h`
- V2 active print path:
  `C:\wzp\Holographicface\按帧送图V2\Print\Test9030Dlg.cpp`, function `start_new`
- V2 presenter:
  `C:\wzp\Holographicface\按帧送图V2\Print\D3DImageRenderer.{h,cpp}` and
  `SecondScr.{h,cpp}`
- V2 current axis defaults:
  `C:\wzp\Holographicface\按帧送图V2\Print\aixs_config.cfg`
- V2 current main defaults:
  `C:\wzp\Holographicface\按帧送图V2\Print\main_config.cfg`
- Approved design:
  `docs/superpowers/specs/2026-07-30-imc60g-x64-print-module-design.md`

---

### Task 1: Vendor the x64 SDK and make dependency verification fail closed

**Files:**
- Create: `vendor/imc60g/include/IMC_Library.h`
- Create: `vendor/imc60g/include/errorcode.h`
- Create: `vendor/imc60g/lib/x64/IMC_Library_x64.lib`
- Create: `vendor/imc60g/bin/x64/IMC_Library_x64.dll`
- Create: `Pri/imc60g.pri`
- Create: `scripts/verify_imc60g_sdk.ps1`
- Create: `scripts/run_qmake_test.ps1`
- Modify: `mergeholo.pro`
- Modify: `scripts/build.ps1`

**Interfaces:**
- Consumes: the four SDK files in `IMC_Library_Release`.
- Produces: `IMC60G_INCLUDE_DIR`, `IMC60G_LIB_DIR`, `IMC60G_RUNTIME_DLL`, and a build that always stages `IMC_Library_x64.dll`.

- [ ] **Step 1: Write the failing SDK verifier**

Create `scripts/verify_imc60g_sdk.ps1` with these required checks:

```powershell
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
```

- [ ] **Step 2: Run it and verify the missing vendored files fail**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify_imc60g_sdk.ps1
```

Expected: FAIL with `Missing IMC60G SDK artifact`.

- [ ] **Step 3: Add the reusable x64 qmake test runner**

Create `scripts/run_qmake_test.ps1`:

```powershell
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
Push-Location $buildDir
try {
    $command = "call `"$vs`" -arch=x64 -host_arch=x64 >nul && " +
        "`"$qmake`" `"$projectPath`" `"CONFIG+=release`" -o Makefile && " +
        "nmake /NOLOGO /F Makefile.Release"
    cmd /d /c $command
    if ($LASTEXITCODE -ne 0) { throw "qmake test build failed: $Project" }
    $exe = Join-Path $buildDir "release\$target.exe"
    if (-not (Test-Path -LiteralPath $exe)) { throw "Missing test executable: $exe" }
    $env:QT_QPA_PLATFORM = "offscreen"
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "qmake test failed: $target" }
}
finally {
    Pop-Location
}
```

- [ ] **Step 4: Copy the exact SDK artifacts into the repository**

Run:

```powershell
$source = "C:\wzp\Holographicface\按帧送图V2\IMC_Library_Release"
New-Item -ItemType Directory -Force vendor\imc60g\include, vendor\imc60g\lib\x64, vendor\imc60g\bin\x64 | Out-Null
Copy-Item -LiteralPath "$source\IMC_Library.h" -Destination vendor\imc60g\include\IMC_Library.h
Copy-Item -LiteralPath "$source\errorcode.h" -Destination vendor\imc60g\include\errorcode.h
Copy-Item -LiteralPath "$source\x64\IMC_Library_x64.lib" -Destination vendor\imc60g\lib\x64\IMC_Library_x64.lib
Copy-Item -LiteralPath "$source\x64\IMC_Library_x64.dll" -Destination vendor\imc60g\bin\x64\IMC_Library_x64.dll
```

- [ ] **Step 5: Add one qmake SDK definition**

Create `Pri/imc60g.pri`:

```qmake
IMC60G_ROOT = $$clean_path($$PWD/../vendor/imc60g)
IMC60G_INCLUDE_DIR = $$IMC60G_ROOT/include
IMC60G_LIB_DIR = $$IMC60G_ROOT/lib/x64
IMC60G_RUNTIME_DLL = $$IMC60G_ROOT/bin/x64/IMC_Library_x64.dll

!exists($$IMC60G_INCLUDE_DIR/IMC_Library.h): error("Missing IMC60G header")
!exists($$IMC60G_LIB_DIR/IMC_Library_x64.lib): error("Missing IMC60G x64 import library")
!exists($$IMC60G_RUNTIME_DLL): error("Missing IMC60G x64 runtime DLL")

INCLUDEPATH += $$IMC60G_INCLUDE_DIR
DEPENDPATH += $$IMC60G_INCLUDE_DIR
win32-msvc*:LIBS += /LIBPATH:$$shell_path($$IMC60G_LIB_DIR) IMC_Library_x64.lib
```

Add `include(Pri/imc60g.pri)` to `mergeholo.pro`. In `scripts/build.ps1`, after the executable is copied to `00-bin`, add:

```powershell
$imcRuntime = Join-Path $repoRoot "vendor\imc60g\bin\x64\IMC_Library_x64.dll"
if (-not (Test-Path -LiteralPath $imcRuntime)) { throw "Missing IMC runtime: $imcRuntime" }
Copy-Item -LiteralPath $imcRuntime -Destination (Join-Path $outputDir "IMC_Library_x64.dll") -Force
& powershell -ExecutionPolicy Bypass -File (Join-Path $repoRoot "scripts\verify_imc60g_sdk.ps1") `
    -RepoRoot $repoRoot -RuntimeDirectory $outputDir
if ($LASTEXITCODE -ne 0) { throw "IMC60G SDK verification failed" }
```

- [ ] **Step 6: Verify the SDK and clean x64 linkage**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify_imc60g_sdk.ps1
```

Expected: `IMC60G x64 SDK verification passed.`

- [ ] **Step 7: Commit**

```powershell
git add vendor/imc60g Pri/imc60g.pri scripts/verify_imc60g_sdk.ps1 scripts/run_qmake_test.ps1 scripts/build.ps1 mergeholo.pro
git commit -m "build: add verified IMC60G x64 SDK"
```

---

### Task 2: Lock the V2 production profile and migrate editable settings

**Files:**
- Create: `printing/PrintHardwareProfile.h`
- Create: `printing/PrintHardwareProfile.cpp`
- Create: `config/imc60g_print.ini`
- Modify: `printing/PrintConfig.h`
- Modify: `printing/PrintConfig.cpp`
- Modify: `printing/tests/test_printing_modules.cpp`
- Modify: `printing/tests/printing_tests.pro`

**Interfaces:**
- Produces: `PrintHardwareProfile loadPrintHardwareProfile(const QString&, QString*)`.
- Produces: `bool validatePrintHardwareProfile(const PrintHardwareProfile&, QString*)`.
- Produces: version 2 `Print9030Config` with V2 IMC defaults.

- [ ] **Step 1: Add failing exact-profile tests**

Add to `printing/tests/test_printing_modules.cpp`:

```cpp
void testImc60gProductionProfileMatchesV2()
{
    QString error;
    const PrintHardwareProfile p = loadPrintHardwareProfile(
        QDir::current().absoluteFilePath("../../config/imc60g_print.ini"), &error);
    expect(error.isEmpty(), "hardware profile should load");
    expect(p.version == 1 && p.cardIndex == 0, "card profile should be versioned");
    expect(p.axisX == 1 && p.axisY == 0, "X/Y mapping must match V2");
    expect(p.homeOrder == QVector<PrintHardwareProfile::LogicalAxis>({
        PrintHardwareProfile::LogicalAxis::Y,
        PrintHardwareProfile::LogicalAxis::X
    }), "home order must be logical Y then logical X");
    expect(p.homeDirectionX == -1 && p.homeDirectionY == -1, "home direction must match V2");
    expect(p.homeBackoffX == 28000 && p.homeBackoffY == 92000, "backoff pulses must match current V2 source");
    expect(p.printStepPulse == 1000 && p.forwardDelayPulse == 4000, "IMC print basis must match V2");
    expect(p.reverseFixedPulse == 2000 && p.exposureOffsetPulse == 2000, "IMC timing constants must match V2");
    expect(p.sv660nDoFunction == 25 && p.sv660nPointIndex == 1, "SV660N DO1 mapping must match V2");
    expect(p.sv660nPositiveAttribute == 129 && p.sv660nNegativeAttribute == 130,
        "SV660N crossing attributes must match V2");
}
```

Also change the old defaults test to expect:

```cpp
expect(config.axisX.subdivision == 40 && config.axisX.resolution == 50, "X must use 2000 units/mm");
expect(config.axisX.speedOfMovement == 5000, "X speed must match V2 aixs_config.cfg");
expect(config.axisY.speedOfMovement == 60000, "Y speed must match V2 aixs_config.cfg");
expect(config.main.moveAdjustMm == 20.0, "move adjustment must match V2 main_config.cfg");
expect(config.main.widthScale == 3.8 && config.main.heightScale == 2.8, "scale must match V2");
expect(config.main.addTempPulse == 16000 && config.main.leadPulse == 1000, "IMC pulse defaults must match V2");
```

- [ ] **Step 2: Run tests and verify failure**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing\tests\printing_tests.pro
```

Expected: compile failure for missing `PrintHardwareProfile`, followed by
assertion failures for old DFJZH defaults after the type exists.

- [ ] **Step 3: Define the immutable profile**

Create `printing/PrintHardwareProfile.h`:

```cpp
#pragma once
#include <QString>
#include <QVector>

struct PrintHardwareProfile {
    enum class LogicalAxis { X, Y, Z, R };
    int version = 1;
    int cardIndex = 0;
    int axisX = 1;
    int axisY = 0;
    QVector<LogicalAxis> homeOrder = {LogicalAxis::Y, LogicalAxis::X};
    int homeDirectionX = -1;
    int homeDirectionY = -1;
    int homeSpeed = 32000;
    int homeAcceleration = 80000;
    int homeDeceleration = 80000;
    int homeTimeoutMs = 180000;
    int homeStableMs = 500;
    int homeMinimumMove = 100;
    int homeBackoffX = 28000;
    int homeBackoffY = 92000;
    int homeBackoffSpeed = 10000;
    int homeBackoffTimeoutMs = 30000;
    long printStepPulse = 1000;
    long forwardDelayPulse = 4000;
    long reverseFixedPulse = 2000;
    long exposureOffsetPulse = 2000;
    int sv660nDoFunction = 25;
    int sv660nPointIndex = 1;
    int sv660nMode = 0;
    int sv660nWidth = 1000;
    bool sv660nUserUnits = true;
    int sv660nPositiveAttribute = 129;
    int sv660nNegativeAttribute = 130;
};

PrintHardwareProfile loadPrintHardwareProfile(const QString& path, QString* errorMessage = nullptr);
bool validatePrintHardwareProfile(const PrintHardwareProfile& profile, QString* errorMessage = nullptr);
```

Implement strict INI loading in `PrintHardwareProfile.cpp`: every key above is
required, unknown versions fail, X and Y must differ, `axisX` must equal 1,
`axisY` must equal 0, home order must be `Y,X`, and the exposure values must
equal the approved production profile.

- [ ] **Step 4: Add the exact versioned configuration**

Create `config/imc60g_print.ini`:

```ini
[profile]
version=1
card_index=0
axis_x=1
axis_y=0
home_order=Y,X

[homing]
direction_x=-1
direction_y=-1
speed=32000
acceleration=80000
deceleration=80000
timeout_ms=180000
stable_ms=500
minimum_move_pulse=100
backoff_x_pulse=28000
backoff_y_pulse=92000
backoff_speed=10000
backoff_timeout_ms=30000

[print]
step_pulse=1000
forward_delay_pulse=4000
reverse_fixed_pulse=2000
default_add_temp_pulse=16000
default_lead_pulse=1000
exposure_offset_pulse=2000

[sv660n]
do_function=25
point_index=1
mode=0
width=1000
use_user_unit=true
positive_attribute=129
negative_attribute=130
```

- [ ] **Step 5: Update editable defaults and persistence**

Set `defaultPrint9030Config()` to the exact current V2 `main_config.cfg` and
`aixs_config.cfg` values. Add `configVersion = 2` to `Print9030Config`; save it
under `[meta] version=2`. On an older file, migrate existing user values but
replace the hardware-card family with IMC60G and never import
`axisW.electricalStatus` as a simulated shutter.

- [ ] **Step 6: Run tests**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing\tests\printing_tests.pro
```

Expected: all profile/default/persistence tests pass.

- [ ] **Step 7: Commit**

```powershell
git add printing/PrintHardwareProfile.* printing/PrintConfig.* config/imc60g_print.ini printing/tests
git commit -m "feat: define V2 IMC60G production profile"
```

---

### Task 3: Implement native IMC API and explicit connect/home/manual motion

**Files:**
- Create: `printing/IImc60gApi.h`
- Create: `printing/Imc60gApi.h`
- Create: `printing/Imc60gApi.cpp`
- Create: `printing/Imc60gMotionController.h`
- Create: `printing/Imc60gMotionController.cpp`
- Create: `printing/tests/test_imc60g_motion.cpp`
- Create: `printing/tests/imc60g_motion_tests.pro`
- Modify: `mergeholo.pro`

**Interfaces:**
- Consumes: `PrintHardwareProfile`, `PrintAxisConfig`.
- Produces: `Imc60gConnectionState`, `Imc60gAxisSnapshot`.
- Produces: `connectAndHome()`, `disconnect()`, `moveRelative()`, `stopAxis()`,
  `readSnapshot()`, and `isReadyForPrint()`.

- [ ] **Step 1: Define the narrow mockable API**

Create `IImc60gApi.h` with explicit wrapper methods:

```cpp
class IImc60gApi {
public:
    virtual ~IImc60gApi() = default;
    virtual int getCardsNum(unsigned int* count) = 0;
    virtual int openCard(unsigned int cardIndex) = 0;
    virtual int closeCard(unsigned int cardIndex) = 0;
    virtual int scanEthercat(unsigned int cardIndex, short waitSeconds) = 0;
    virtual int initEthercat(unsigned int cardIndex) = 0;
    virtual int startEthercat(unsigned int cardIndex) = 0;
    virtual int stopEthercat(unsigned int cardIndex) = 0;
    virtual int setEmergencyLevel(unsigned int cardIndex, short inverted) = 0;
    virtual int clearAxisStatus(unsigned int cardIndex, short axis) = 0;
    virtual int servoOn(unsigned int cardIndex, short axis) = 0;
    virtual int servoOff(unsigned int cardIndex, short axis) = 0;
    virtual int setMotionProfile(unsigned int cardIndex, short axis, double velocity,
        double acceleration, double deceleration, double startVelocity, double endVelocity) = 0;
    virtual int startPtp(unsigned int cardIndex, short axis, int target) = 0;
    virtual int startJog(unsigned int cardIndex, short axis, int direction) = 0;
    virtual int stop(unsigned int cardIndex, short axis, int mode) = 0;
    virtual int axisStatus(unsigned int cardIndex, short axis, unsigned int* status) = 0;
    virtual int stopReason(unsigned int cardIndex, short axis, unsigned int* reason) = 0;
    virtual int plannedPosition(unsigned int cardIndex, short axis, int* position) = 0;
    virtual int encoderPosition(unsigned int cardIndex, short axis, int* position) = 0;
    virtual int setCurrentPosition(unsigned int cardIndex, short axis, double position) = 0;
    virtual int syncPosition(unsigned int cardIndex, short axis) = 0;
    virtual int setAxisSdo(unsigned int cardIndex, short axis, unsigned short index,
        unsigned short subIndex, unsigned char* data, unsigned int size, unsigned int* abortCode) = 0;
    virtual int getAxisSdo(unsigned int cardIndex, short axis, unsigned short index,
        unsigned short subIndex, unsigned char* data, unsigned int size,
        unsigned int* resultSize, unsigned int* abortCode) = 0;
};
```

`Imc60gApi.cpp` must be a one-to-one adapter to `IMC_Library.h`; it may translate
types but not invent success return codes.

- [ ] **Step 2: Write failing lifecycle tests**

In `test_imc60g_motion.cpp`, implement a `RecordingImc60gApi` and assert:

```cpp
expect(controller.state() == Imc60gConnectionState::Disconnected, "starts disconnected");
expect(controller.connectAndHome(&error), "connect and home should pass with recording API");
expect(api.events == QStringList({
    "cards","open:0","scan:0:40","ecat_init:0","ecat_start:0",
    "emg_level:0:1","clear:0","clear:1","servo_on:0","servo_on:1",
    "home:0:-1","backoff:0:92000","zero:0",
    "home:1:-1","backoff:1:28000","zero:1"
}), "connection and Y/X homing order must match V2");
expect(controller.state() == Imc60gConnectionState::Ready, "successful homing is ready");
```

Add failure cases for card count, open, EtherCAT, Servo, Y home, X home, backoff,
and zero. Each must leave both axes stopped, Servo Off when it was enabled,
EtherCAT stopped, and the card closed.

- [ ] **Step 3: Run tests and verify compile failure**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing\tests\imc60g_motion_tests.pro
```

Expected: missing controller/API types.

- [ ] **Step 4: Port the V2 connect and homing behavior**

Implement the exact V2 logic from `EnsureImc60gOpen`,
`LimitHomeImc60gAxesOnOpen`, and `LimitHomeSingleOldAxis`, but expose it only
through `connectAndHome()`. Do not call it from constructors or dialog setup.

Use bounded polling at 1 ms, record position and stop reason, require configured
minimum travel, perform the exact X/Y backoff pulse values, then:

```cpp
api_->setCurrentPosition(card, axis, 0.0);
QThread::msleep(20);
api_->syncPosition(card, axis);
api_->setCurrentPosition(card, axis, 0.0);
```

Every nonzero return becomes a `false` result with function name, card, physical
axis, numeric code, and `errorcode.h` text.

- [ ] **Step 5: Add manual movement tests and implementation**

Test `moveRelative(LogicalAxis::X, +10.0, axisX, &error)` calculates
`10 * 40 * 50 = 20000` units and calls physical axis 1. Test Y maps to physical
axis 0. Test maximum travel, disconnected state, active print, missing real
axis, and invalid speed all fail without `startPtp`.

- [ ] **Step 6: Run the motion tests**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing\tests\imc60g_motion_tests.pro
```

Expected: all recording API tests pass without loading the vendor DLL or moving
hardware.

- [ ] **Step 7: Commit**

```powershell
git add printing/IImc60gApi.h printing/Imc60gApi.* printing/Imc60gMotionController.* printing/tests/imc60g_motion_tests.pro printing/tests/test_imc60g_motion.cpp mergeholo.pro
git commit -m "feat: add explicit IMC60G connection and motion control"
```

---

### Task 4: Port the exact SV660N DO1 comparison backend

**Files:**
- Create: `printing/Sv660nExposureController.h`
- Create: `printing/Sv660nExposureController.cpp`
- Create: `printing/tests/test_sv660n_exposure.cpp`
- Create: `printing/tests/sv660n_exposure_tests.pro`
- Modify: `mergeholo.pro`

**Interfaces:**
- Consumes: `IImc60gApi`, card index, physical Y axis, `PrintHardwareProfile`.
- Produces: `bool arm(long begin, long end, QString*)`.
- Produces: `bool disarm(QString*)` and `bool isArmed() const`.

- [ ] **Step 1: Write the failing positive-direction SDO sequence test**

With planned position 1000 and requested window `[3000, LONG_MAX]`, assert:

```cpp
expect(exposure.arm(3000, LONG_MAX, &error), "positive arm should succeed");
expect(api.sdoEvents == QStringList({
    "u16:axis0:2004:01:25",
    "u16:axis0:2018:01:0",
    "u16:axis0:2018:13:256",
    "s32:axis0:2018:0D:0",
    "u16:axis0:2018:05:0",
    "u16:axis0:2018:05:1",
    "u16:axis0:2018:05:0",
    "u16:axis0:2018:04:0",
    "u16:axis0:2018:06:1000",
    "u16:axis0:2018:08:1",
    "u16:axis0:2018:09:1",
    "s32:axis0:2019:01:2000",
    "u16:axis0:2019:03:129",
    "u16:axis0:2018:01:1"
}), "positive crossing SDO sequence must be byte-for-byte equivalent to V2");
expect(api.readbackEvents == QStringList({
    "u16:axis0:2004:01:25",
    "u16:axis0:2018:13:256",
    "u16:axis0:2018:04:0",
    "u16:axis0:2018:06:1000",
    "u16:axis0:2018:08:1",
    "u16:axis0:2018:09:1",
    "s32:axis0:2019:01:2000",
    "u16:axis0:2019:03:129"
}), "stable SV660N settings must be read back before compare enable");
```

- [ ] **Step 2: Add reverse, failure, and disarm tests**

For current position 9000 and requested window `[LONG_MIN, 7000]`, expect target
`-2000` and attribute `130`. Inject failure and nonzero abort code at every SDO
write; `arm()` must fail and call compare-enable-off. `disarm()` must write:

```text
u16:axis0:2018:01:0
```

- [ ] **Step 3: Run tests and verify missing implementation**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing\tests\sv660n_exposure_tests.pro
```

Expected: compile failure.

- [ ] **Step 4: Port `ArmSv660nDriveCompare` and disarm**

Port only the active V2 SV660N branch. Preserve:

- nearest boundary selection;
- relative int32 range check;
- target sub-index `1 + (point - 1) * 3`;
- attribute sub-index `3 + (point - 1) * 3`;
- H18.04 edge sequence with 5 ms between set and release;
- every exact SDO index, sub-index, width, mode, attribute, and enable order.

After writing all stable H04/H18/H19 values and before enabling H18.00, read
them through `IMC_GetEcatAxSdo`. Require `resultSize` to match the expected
16-bit or 32-bit value and require exact equality. H18.04 is an edge command and
is not read back. A readback failure, SDO abort, short result, or mismatch must
leave H18.00 disabled and fail preflight.

Never compile the SOFTWARE_SIM, LOCAL_DO, LOCAL_PSO_WINDOW, or MULTI_AX_CMP
branches into the production controller.

- [ ] **Step 5: Run tests**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing\tests\sv660n_exposure_tests.pro
```

Expected: all exact-sequence and failure-injection cases pass.

- [ ] **Step 6: Commit**

```powershell
git add printing/Sv660nExposureController.* printing/tests/test_sv660n_exposure.cpp printing/tests/sv660n_exposure_tests.pro mergeholo.pro
git commit -m "feat: port SV660N DO1 position comparison"
```

---

### Task 5: Replace legacy timing with V2 IMC golden calculations

**Files:**
- Create: `printing/V2PrintTiming.h`
- Create: `printing/V2PrintTiming.cpp`
- Create: `printing/tests/test_v2_print_timing.cpp`
- Create: `printing/tests/v2_print_timing_tests.pro`
- Delete after replacement: `printing/LegacyPrintTiming.h`
- Delete after replacement: `printing/LegacyPrintTiming.cpp`
- Modify: `mergeholo.pro`

**Interfaces:**
- Produces: `V2PrintPlan buildV2PrintPlan(const Print9030Config&, const PrintHardwareProfile&, double refreshHz, QString*)`.
- Produces: immutable `V2RowPlan` entries with direction, targets, comparison window, frame order, delay frames, and expected positions.

- [ ] **Step 1: Define plan types and failing golden tests**

Define:

```cpp
struct V2RowPlan {
    int row = 0;
    bool reverse = false;
    qint64 yStart = 0;
    qint64 yTarget = 0;
    qint64 constantBegin = 0;
    qint64 exposureBegin = 0;
    qint64 compareBegin = 0;
    qint64 compareEnd = 0;
    QVector<int> logicalFrameOrder;
    int startDelayFrames = 0;
    int holdFramesAfterPresent = 0;
};
struct V2PrintPlan {
    qint64 stepPulse = 0;
    qint64 accelerationPulse = 0;
    qint64 exposurePulse = 0;
    qint64 totalPulse = 0;
    int framesPerImage = 0;
    QVector<V2RowPlan> rows;
};
```

Create fixed golden cases from current V2 defaults:

```cpp
Print9030Config c = defaultPrint9030Config();
c.main.gridRows = 2;
c.main.gridColumns = 3;
const V2PrintPlan p = buildV2PrintPlan(c, profile, 60.0, &error);
expect(p.stepPulse == 1000, "IMC V2 step basis is 1000");
expect(p.rows[0].logicalFrameOrder == QVector<int>({0,1,2}), "first forward row frame order matches V2");
expect(p.rows[1].logicalFrameOrder == QVector<int>({2,1,0}), "second reverse row frame order matches V2");
expect(p.rows[0].compareBegin == p.rows[0].exposureBegin,
    "active IMC60G forward compare uses the finite exposure window");
expect(p.rows[1].compareEnd == p.rows[1].exposureBegin,
    "active IMC60G reverse compare normalizes the finite exposure window");
```

Add boundary tests for `addTempPulse=16000`, `leadPulse=1000`,
`forwardDelayPulse=4000`, `reverseFixedPulse=2000`,
`exposureOffsetPulse=2000`, integer VBlank fit, negative delay, qint32 overflow,
zero acceleration, and invalid/pathological grid sizes. Source-frame count is
not an input to this pure timing interface and is validated by Task 7 preflight.

The `LONG_MIN/LONG_MAX` comparison windows in V2 `start_new` are guarded by
`if (!Motion_IsImc60gMode())` and belong only to the rejected old-card path.
The production IMC60G plan must preserve the finite
`yExposeBeginPos/yExposeEndPos` window, including the exposure offset.

- [ ] **Step 2: Run tests and verify failure**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing\tests\v2_print_timing_tests.pro
```

Expected: missing `V2PrintTiming`.

- [ ] **Step 3: Port the pure calculations from `start_new`**

Move only calculations into `V2PrintTiming.cpp`. Preserve V2 integer rounding and
branch conditions. Use checked `qint64` intermediates, then reject values outside
the int32 SDK range instead of truncating.

Logically distinguish V2's `bForward` naming from physical direction by storing
the unambiguous `reverse` boolean and documenting the correspondence in one
comment next to the port.

- [ ] **Step 4: Run golden tests**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing\tests\v2_print_timing_tests.pro
```

Expected: all fixed cases pass and old 800-pulse DFJZH expectations are absent.

- [ ] **Step 5: Commit**

```powershell
git add printing/V2PrintTiming.* printing/tests/test_v2_print_timing.cpp printing/tests/v2_print_timing_tests.pro mergeholo.pro
git rm printing/LegacyPrintTiming.h printing/LegacyPrintTiming.cpp
git commit -m "feat: port V2 IMC print timing"
```

---

### Task 6: Port the latest V2 D3D11/DXGI presentation and VBlank path

**Files:**
- Create: `printing/V2D3DFramePresenter.h`
- Create: `printing/V2D3DFramePresenter.cpp`
- Create: `printing/IVBlankWaiter.h`
- Modify: `printing/IPrintFramePresenter.h`
- Modify: `printing/SecondScreenSelection.h`
- Modify: `printing/SecondScreenSelection.cpp`
- Create: `printing/tests/test_v2_presenter_contract.cpp`
- Create: `printing/tests/v2_presenter_contract_tests.pro`
- Delete after replacement: `printing/LegacyD3DImageRenderer.{h,cpp}`
- Delete after replacement: `printing/LegacySecondScreenPresenter.{h,cpp}`
- Modify: `mergeholo.pro`

**Interfaces:**
- Consumes: immutable `PrintFrame`.
- Produces: `prepare`, `present`, `waitForDisplayFrame`, DXGI diagnostics, and deterministic `shutdown`.

- [ ] **Step 1: Write failing presenter-contract tests**

Use injected `IVBlankWaiter` and test:

```cpp
expect(!presenter.prepare(frame, size, &error), "primary-only display must fail");
expect(!motionStarted, "display preflight failure must precede motion");
expect(presenter.selectedMonitorIndex(displays) == 2, "last non-primary display matches V2");
expect(presenter.prepare(validBgr24, size, &error), "BGR24 frame is supported");
expect(presenter.present(validBgra32, size, &error), "BGRA32 frame is supported");
expect(!presenter.present(badStride, size, &error), "invalid stride fails before upload");
```

Add a test that `waitForDisplayFrame()` propagates the injected DXGI failure and
never converts it to success through `DwmFlush`.

- [ ] **Step 2: Run tests and verify failure**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing\tests\v2_presenter_contract_tests.pro
```

Expected: missing V2 presenter.

- [ ] **Step 3: Port the current V2 renderer**

Port the current `D3DImageRenderer` device, swap chain, texture upload, render,
Present, frame-statistics, waitable-object diagnostics, and physical-output
VBlank logic. Preserve the current V2 output selection and row-anchor behavior.

Do not port MFC window ownership. Create the native presentation window through
Qt/Win32 on the GUI thread, then serialize renderer commands through one
presentation owner. The print worker waits synchronously for completion so it
cannot advance before Present/VBlank.

- [ ] **Step 4: Run contract tests and a non-motion display smoke test**

Add `--print-display-smoke` to the test executable. In that mode it creates ten
numbered `QImage` frames in memory, presents them on the selected second screen,
waits VBlank, logs DXGI frame statistics, and never constructs
`Imc60gMotionController`.

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing\tests\v2_presenter_contract_tests.pro
& .\FF-tmp\qmake-tests\v2_presenter_contract_tests\release\v2_presenter_contract_tests.exe --print-display-smoke
```

Expected: contract tests pass; smoke test exits 0 and logs ten ordered presents.

- [ ] **Step 5: Remove legacy presenter sources and commit**

```powershell
git add printing/V2D3DFramePresenter.* printing/IVBlankWaiter.h printing/IPrintFramePresenter.h printing/SecondScreenSelection.* printing/tests mergeholo.pro
git rm printing/LegacyD3DImageRenderer.* printing/LegacySecondScreenPresenter.*
git commit -m "feat: port V2 DXGI frame presentation"
```

---

### Task 7: Build fail-closed preflight and the V2 print state machine

**Files:**
- Create: `printing/PrintHardwarePreflight.h`
- Create: `printing/PrintHardwarePreflight.cpp`
- Modify: `printing/IMotionController.h`
- Modify: `printing/PrintJobRunner.h`
- Modify: `printing/PrintJobRunner.cpp`
- Create: `printing/tests/test_v2_print_engine.cpp`
- Create: `printing/tests/v2_print_engine_tests.pro`
- Delete after replacement: `printing/DfjzhMotionController.{h,cpp}`
- Modify: `mergeholo.pro`

**Interfaces:**
- Consumes: ready `Imc60gMotionController`, `Sv660nExposureController`,
  `V2PrintPlan`, `PrintImageSet`, and `IPrintFramePresenter`.
- Produces: states `Ready`, `Printing`, `Paused`, `Stopping`, `Fault`.
- Produces: `start`, `pause`, `resume`, `cancel`, `finished`, and structured progress/log signals.

- [ ] **Step 1: Write the preflight matrix tests**

Create one test per check. Each injected failure must assert zero calls to
`startPtp` and zero exposure-enable writes:

```cpp
expectFailure(PreflightFault::SdkRuntime, "SDK");
expectFailure(PreflightFault::Ethercat, "EtherCAT");
expectFailure(PreflightFault::AxisMapping, "X=1, Y=0");
expectFailure(PreflightFault::Servo, "Servo");
expectFailure(PreflightFault::Homing, "回零");
expectFailure(PreflightFault::ExposureProfile, "SV660N");
expectFailure(PreflightFault::ImageCount, "图像");
expectFailure(PreflightFault::SecondScreen, "第二屏");
expectFailure(PreflightFault::VBlank, "VBlank");
```

- [ ] **Step 2: Write exact two-row event-order tests**

Recording fakes must produce:

```text
preflight
present:0
vblank
arm:forward
move_y:forward
present:0,vblank,present:1,vblank,present:2,vblank
wait_y
disarm
move_x
wait_x
present:2
vblank
arm:reverse
move_y:reverse
present:2,vblank,present:1,vblank,present:0,vblank
wait_y
disarm
cleanup_stop
return_xy_zero
presenter_shutdown
finished_success
```

The actual assertion uses `QStringList`, not substring matching.

- [ ] **Step 3: Add pause, cancel, and injected-failure tests**

Pause must reach a defined safe boundary, disarm exposure, and enter `Paused`.
Resume re-runs the dynamic hardware state checks before the next row/segment.
Cancel during acceleration, constant speed, Present, VBlank, Y wait, and X move
must all produce:

```text
disarm -> stop active axes -> wait stopped -> conditional return to zero -> presenter shutdown
```

If safe return is impossible, report failure and keep axes stopped; never claim
success.

- [ ] **Step 4: Run tests and verify old runner fails expectations**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing\tests\v2_print_engine_tests.pro
```

Expected: failure because current runner initializes DFJZH internally and lacks
the required explicit-ready/pause/resume states.

- [ ] **Step 5: Implement preflight and runner**

Replace the factory-created card lifecycle in `PrintJobRunner` with injected,
already-connected IMC60G services owned by `PrintController`. Port the active V2
row loop using `V2PrintPlan`. Preserve synchronous row-anchor presentation,
physical VBlank pacing, exposure arming before Y motion, position-based phase
transitions, and common cleanup.

The runner may not open or close the card. It leaves the connected controller in
`Ready` after a successful or safely cancelled job.

- [ ] **Step 6: Remove DFJZH implementation and run all printing tests**

Expected: no source, include, library, or runtime reference contains
`Dfjzh`, `dfjzh`, `CH365`, or `6052`.

Run:

```powershell
rg -n -i "dfjzh|ch365|6052" printing widgets ui mergeholo.pro Pri scripts
```

Expected: no matches.

- [ ] **Step 7: Commit**

```powershell
git add printing/PrintHardwarePreflight.* printing/IMotionController.h printing/PrintJobRunner.* printing/tests mergeholo.pro
git rm printing/DfjzhMotionController.*
git commit -m "feat: replace print runner with V2 IMC state machine"
```

---

### Task 8: Add the serialized controller and complete IMC60G Qt UI

**Files:**
- Create: `printing/PrintController.h`
- Create: `printing/PrintController.cpp`
- Modify: `widgets/Print9030Dialog.h`
- Modify: `widgets/Print9030Dialog.cpp`
- Replace: `ui/Print9030Dialog.ui`
- Create: `widgets/tests/test_print9030_dialog.cpp`
- Create: `widgets/tests/print9030_dialog_tests.pro`
- Modify: `mergeholo.pro`

**Interfaces:**
- Consumes: hardware profile, editable config, image source, presenter, motion,
  exposure, preflight, and runner.
- Produces: one Qt-facing state model and commands for all buttons.

- [ ] **Step 1: Write failing UI state tests**

Use an injected fake `PrintController` and locate controls by stable object name:

```cpp
expect(!dialog.findChild<QPushButton*>("startButton")->isEnabled(), "print disabled while disconnected");
click("connectHomeButton");
expect(controller.events == QStringList({"connectAndHome"}), "explicit button owns hardware startup");
controller.publishState(PrintUiState::Ready);
expect(button("startButton")->isEnabled(), "print enabled only when ready and source valid");
expect(button("disconnectButton")->isEnabled(), "disconnect enabled when ready");
expect(button("pauseButton")->isEnabled() == false, "pause disabled before printing");
```

Add tests for every retained control:

- source radio buttons and folder selection;
- preview;
- all main and axis parameter fields;
- X/Y positive, negative, and stop;
- connect/home and disconnect;
- start, pause, resume, cancel, close;
- live positions, controller/EtherCAT/Servo/home indicators;
- progress and error detail;
- parameter locking during printing;
- close during printing waits for `safeStopCompleted`.

- [ ] **Step 2: Run UI tests and verify failure**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project widgets\tests\print9030_dialog_tests.pro
```

Expected: missing controls and state behavior.

- [ ] **Step 3: Implement `PrintController` serialization**

Define:

```cpp
enum class PrintUiState {
    Disconnected, Connecting, Homing, Ready, Printing, Paused, Stopping, Fault
};
```

All public commands are Qt slots. They enqueue work onto one controller thread;
only that thread calls motion/exposure/runner methods. Position polling is also a
queued command and is suppressed while a higher-priority SDK operation is active.

- [ ] **Step 4: Replace the UI**

Build these groups in `Print9030Dialog.ui`:

1. image source and preview;
2. immutable hardware summary (`Card0`, `X→Axis1`, `Y→Axis0`,
   `SV660N DO1`);
3. connection/EtherCAT/Servo/home status and live X/Y position;
4. manual motion with step, X-/X+/Y-/Y+/stop;
5. print geometry/timing/pulse parameters;
6. axis profiles;
7. start/pause/resume/cancel/progress/status/error details.

Use typed spin boxes with units and ranges. Do not retain hidden MFC test
buttons, old-card selection, or fake Z/W controls. If later physical axes are
discovered, show them as unavailable until an explicit reviewed mapping exists.

- [ ] **Step 5: Bind fields and button functions**

`Print9030Dialog.cpp` must have one `applyState(PrintUiState)` method. Every
button connects to exactly one controller command. Every editable field maps to
one `Print9030Config` property and round-trips through the config file. Validation
errors focus the exact control and prevent command dispatch.

- [ ] **Step 6: Run UI tests**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project widgets\tests\print9030_dialog_tests.pro
```

Expected: all button, field, state, persistence, and close tests pass under
`QT_QPA_PLATFORM=offscreen`.

- [ ] **Step 7: Commit**

```powershell
git add printing/PrintController.* widgets/Print9030Dialog.* ui/Print9030Dialog.ui widgets/tests/test_print9030_dialog.cpp widgets/tests/print9030_dialog_tests.pro mergeholo.pro
git commit -m "feat: replace printing UI with IMC60G controls"
```

---

### Task 9: Integrate build, deployment, logging, and full non-hardware verification

**Files:**
- Modify: `mergeholo.pro`
- Modify: `scripts/build.ps1`
- Modify: `apps/mergeholo_main.cpp`
- Modify: `widgets/CaptureWindow.cpp`
- Modify: `printing/tests/printing_tests.pro`
- Create: `scripts/test_printing.ps1`
- Create: `docs/imc60g-print-operator.md`

**Interfaces:**
- Produces: one reproducible x64 build and one non-hardware verification command.

- [ ] **Step 1: Add a failing aggregate verification script**

Create `scripts/test_printing.ps1` that:

```powershell
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$vsDevCmd = "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
$qmake = "C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe"
$buildRoot = Join-Path $repoRoot ("FF-tmp\printing-verification-" + (Get-Date -Format "yyyyMMdd-HHmmss"))
New-Item -ItemType Directory -Force $buildRoot | Out-Null
& powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\verify_imc60g_sdk.ps1"
if ($LASTEXITCODE -ne 0) { throw "SDK verification failed" }
$projects = @(
    "printing\tests\imc60g_motion_tests.pro",
    "printing\tests\sv660n_exposure_tests.pro",
    "printing\tests\v2_print_timing_tests.pro",
    "printing\tests\v2_presenter_contract_tests.pro",
    "printing\tests\v2_print_engine_tests.pro",
    "widgets\tests\print9030_dialog_tests.pro"
)
foreach ($project in $projects) {
    $projectPath = Join-Path $repoRoot $project
    $target = [IO.Path]::GetFileNameWithoutExtension($project)
    $buildDir = Join-Path $buildRoot $target
    New-Item -ItemType Directory -Force $buildDir | Out-Null
    $buildCommand = "call `"$vsDevCmd`" -arch=x64 -host_arch=x64 >nul && " +
        "`"$qmake`" `"$projectPath`" `"CONFIG+=release`" -o Makefile && " +
        "nmake /NOLOGO /F Makefile.Release"
    cmd /d /c $buildCommand
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $project" }
    $testExe = Join-Path $buildDir "release\$target.exe"
    if (-not (Test-Path -LiteralPath $testExe)) { throw "Missing test executable: $testExe" }
    $env:QT_QPA_PLATFORM = "offscreen"
    & $testExe
    if ($LASTEXITCODE -ne 0) { throw "Test failed: $target" }
}
& "$PSScriptRoot\build.ps1"
if ($LASTEXITCODE -ne 0) { throw "Application build failed" }
& powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\verify_imc60g_sdk.ps1" `
    -RuntimeDirectory (Join-Path (Split-Path -Parent $PSScriptRoot) "00-bin")
```

- [ ] **Step 2: Run it and record all current failures**

Expected: failure until every new test project and source is fully linked.

- [ ] **Step 3: Complete source lists and runtime staging**

Remove all deleted legacy source names from `mergeholo.pro`. Add all new
printing sources/headers. Stage:

- `IMC_Library_x64.dll`;
- `config/imc60g_print.ini`;
- editable `config/print_9030.ini`;
- existing Qt/platform/image-format dependencies.

At startup, log executable architecture, absolute IMC DLL path, DLL SHA-256,
hardware profile version, axis mapping, and exposure backend. Do not open the
card at startup.

- [ ] **Step 4: Add operator documentation**

Document:

- connection and homing prerequisites;
- physical emergency-stop and limit requirements;
- exact X/Y mapping and DO1 wiring;
- parameter units;
- UI state meanings;
- safe cancellation and disconnection;
- log locations and error-code interpretation;
- statement that entering the dialog does not move hardware.

- [ ] **Step 5: Run aggregate verification**

Expected:

```text
IMC60G x64 SDK verification passed.
imc60g_motion_tests: PASS
sv660n_exposure_tests: PASS
v2_print_timing_tests: PASS
v2_presenter_contract_tests: PASS
v2_print_engine_tests: PASS
print9030_dialog_tests: PASS
mergeholo x64 build: PASS
runtime staging: PASS
```

- [ ] **Step 6: Commit**

```powershell
git add mergeholo.pro scripts/build.ps1 scripts/test_printing.ps1 apps/mergeholo_main.cpp widgets/CaptureWindow.cpp printing/tests/printing_tests.pro docs/imc60g-print-operator.md
git commit -m "build: integrate IMC60G printing module"
```

---

### Task 10: Execute staged connected-hardware acceptance

**Files:**
- Create: `scripts/run_imc60g_acceptance.ps1`
- Create: `docs/imc60g-print-acceptance.md`
- Create at runtime: `runs/imc60g-acceptance/<timestamp>/acceptance.json`
- Create at runtime: `runs/imc60g-acceptance/<timestamp>/print_flow.log`

**Interfaces:**
- Consumes: the verified `00-bin\mergeholo.exe`, connected IMC60G, EtherCAT
  axes, limit switches, second screen, SV660N DO1, oscilloscope or safe load.
- Produces: operator-confirmed evidence for every acceptance stage.

- [ ] **Step 1: Create a guarded acceptance script**

The script requires an explicit stage:

```powershell
param(
    [ValidateSet("discover","home","xy-small","display","do1","one-row","serpentine","cancel","end-to-end")]
    [string]$Stage,
    [switch]$OperatorConfirmedSafe
)
if (-not $OperatorConfirmedSafe) {
    throw "Confirm physical E-stop, limits, clear travel, and safe DO1 load with -OperatorConfirmedSafe"
}
```

It creates a timestamped evidence directory, records the executable/DLL hashes,
profile, stage, start/end time, exit code, and log path. It never advances to the
next stage automatically.

- [ ] **Step 2: Run discovery**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_imc60g_acceptance.ps1 -Stage discover -OperatorConfirmedSafe
```

Expected evidence: one card, healthy EtherCAT master, logical X=physical 1,
logical Y=physical 0, no motion command.

- [ ] **Step 3: Run explicit home**

Expected evidence: Y negative limit, Y backoff 92000, Y zero, X negative limit,
X backoff 28000, X zero, both Servo states healthy.

- [ ] **Step 4: Run low-speed small XY movement**

Use a reviewed small distance and low speed from the UI. Confirm both physical
directions, measured displacement, stop button, and returned position. Any
unexpected direction or distance stops acceptance.

- [ ] **Step 5: Run display-only validation**

Present numbered frames on the non-primary output. Confirm visible order and
capture DXGI Present/VBlank statistics without constructing a motion command.

- [ ] **Step 6: Validate SV660N DO1**

With oscilloscope or safe load connected, arm one positive and one negative
crossing at low speed. Record H04/H18/H19 writes, target positions, measured DO1
polarity, width, and trigger position. Require compare-enable-off after each run.

- [ ] **Step 7: Run one-row and serpentine prints**

First run one row. Then run a small two-row job and confirm forward/reverse frame
order, X row step, Y targets, VBlank pacing, DO1 triggers, and return behavior.

- [ ] **Step 8: Run pause/resume/cancel/error cleanup**

Verify exposure is disabled before `Paused`, resume rechecks hardware, cancel
stops axes, and injected display/hardware failures do not start the next motion.

- [ ] **Step 9: Run memory and folder end-to-end prints**

Use the same small validated job from both sources. Compare frame count/order,
hardware call log, output timing, and cleanup result.

- [ ] **Step 10: Complete the acceptance record and commit scripts/docs**

Fill `docs/imc60g-print-acceptance.md` with evidence directory links, measured
values, pass/fail for each criterion, and any blocked stage. Do not commit
machine-specific runtime logs unless the repository policy explicitly calls for
them.

```powershell
git add scripts/run_imc60g_acceptance.ps1 docs/imc60g-print-acceptance.md
git commit -m "test: add IMC60G hardware acceptance evidence"
```

---

## Final Verification

After all tasks:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\test_printing.ps1
rg -n -i "dfjzh|ch365|6052|software_sim" printing widgets ui mergeholo.pro Pri scripts config
git status --short
```

Expected:

- all automated tests and the x64 application build pass;
- the legacy/simulation search has no production-code matches;
- only intentional user work remains uncommitted;
- the hardware acceptance document contains evidence for every completed stage;
- the UI opens without motion and all hardware actions require explicit commands.

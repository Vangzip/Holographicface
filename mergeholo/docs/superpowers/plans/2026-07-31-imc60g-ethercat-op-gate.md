# IMC60G EtherCAT OP Gate Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Block servo enablement and homing unless the automatically scanned IMC60G EtherCAT master is OP and exposes the locked X/Y axes.

**Architecture:** Keep `IMC_ScanCardEcat(Card0, 40)` as the only automatic bus-initialisation path. Extend the IMC adapter with a stable master-resource result, then gate `connectAndHome()` on master status `6` and an axis count covering physical axes `0` and `1`. Keep error decoding local to the motion controller.

**Tech Stack:** C++17, Qt Core, qmake test projects, IMC60G x64 SDK.

## Global Constraints

- Do not add a UI control or any manual EtherCAT OP workflow.
- Do not change Card0, X=Axis1, Y=Axis0, Y-then-X homing, emergency, or cleanup behavior.
- OP/resource gate failure must issue no `IMC_ServoOn` and retain stop/EtherCAT-close/card-close cleanup.
- Verification uses mocked SDK tests only; never open a card or move hardware.

---

## File Structure

- `printing/IImc60gApi.h`: SDK-independent master information type and adapter contract.
- `printing/Imc60gApi.h`, `printing/Imc60gApi.cpp`: `TMasterInfo.axisCnt` conversion.
- `printing/Imc60gMotionController.cpp`: OP/resource gate and `0x0702` decoder entry.
- `printing/tests/imc60g_safety_tests.cpp`: safety regressions.
- `printing/tests/test_imc60g_motion.cpp`, `printing/tests/test_sv660n_exposure.cpp`: test-double contract updates.

### Task 1: Create red safety tests

**Files:**

- Modify: `printing/tests/imc60g_safety_tests.cpp:69-165,648-700`
- Modify: `printing/tests/test_imc60g_motion.cpp:29-125`
- Modify: `printing/tests/test_sv660n_exposure.cpp:54-90`

**Interfaces:**

- Consumes: `IImc60gApi::ethercatMasterInfo(unsigned int, Imc60gMasterInfo*)`.
- Produces: a failure proving a non-OP master or insufficient axes cannot reach `servoOn`.

- [ ] **Step 1: Add the new adapter method to test doubles and the intended safety assertions**

```cpp
// Each test double implements this default-success method.
int ethercatMasterInfo(unsigned int, Imc60gMasterInfo* info) override
{
    if (info) info->axisCount = 2;
    return 0;
}

// SafetyApi adds configurable fields.
unsigned int masterStatus = 6;
short masterAxisCount = 2;
```

```cpp
void testEthercatOpGate()
{
    SafetyApi notOperational;
    AdvancingClock clock;
    notOperational.masterStatus = 4;
    Imc60gMotionController controller(&notOperational, PrintHardwareProfile(), &clock);
    QString error;
    check(!controller.connectAndHome(&error), "non-OP master must fail");
    check(error.contains("OP") && !notOperational.events.contains("servo_on:0"),
        "non-OP master must block Servo On: " + error);

    SafetyApi insufficientAxes;
    insufficientAxes.masterAxisCount = 1;
    Imc60gMotionController axisController(&insufficientAxes, PrintHardwareProfile(), &clock);
    check(!axisController.connectAndHome(&error), "one EtherCAT axis must fail");
    check(error.contains("axis") && !insufficientAxes.events.contains("servo_on:0"),
        "insufficient axes must block Servo On: " + error);
}
```

Add `{ "servo_on0", 0x32000702, "IMC_ServoOn", "ERR_NO_SYS_INT_SIGNAL", "missing EtherCAT system interrupt" }` to `connectionErrors`, then call `testEthercatOpGate()` from `runImc60gSafetyTests()`.

- [ ] **Step 2: Run the focused test and observe the red result**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing/tests/imc60g_motion_tests.pro
```

Expected: compilation fails because the interface method does not exist, or the safety executable fails because Servo On was called after a non-OP/one-axis scan.

### Task 2: Expose master axis count through the SDK adapter

**Files:**

- Modify: `printing/IImc60gApi.h:1-17`
- Modify: `printing/Imc60gApi.h:11-21`
- Modify: `printing/Imc60gApi.cpp:61-65`

**Interfaces:**

- Consumes: `IMC_GetEcatMasterInfo(short, TMasterInfo*)`.
- Produces: `Imc60gMasterInfo` containing `axisCount`.

- [ ] **Step 1: Add the stable, SDK-independent interface**

```cpp
struct Imc60gMasterInfo {
    short axisCount = 0;
};

virtual int ethercatMasterInfo(unsigned int cardIndex,
    Imc60gMasterInfo* info) = 0;
```

Declare the same override beside `ethercatMasterStatus` in `Imc60gApi.h`.

- [ ] **Step 2: Convert native `TMasterInfo` at the adapter boundary**

```cpp
int Imc60gApi::ethercatMasterInfo(
    unsigned int cardIndex, Imc60gMasterInfo* info)
{
    TMasterInfo nativeInfo = {};
    const int rc = IMC_GetEcatMasterInfo(cardNumber(cardIndex), &nativeInfo);
    if (rc == 0 && info) {
        info->axisCount = nativeInfo.axisCnt;
    }
    return rc;
}
```

- [ ] **Step 3: Re-run the focused test and verify it reaches the expected gate failure**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing/tests/imc60g_motion_tests.pro
```

Expected: build succeeds, then the new safety assertion fails because the controller has not yet enforced OP/axis discovery.

### Task 3: Enforce automatic OP and resource gate

**Files:**

- Modify: `printing/Imc60gMotionController.cpp:85-190,425-438`

**Interfaces:**

- Consumes: `ethercatMasterStatus` and `ethercatMasterInfo`.
- Produces: no-Servo-On failure when automatic scanning does not produce OP and the locked axes.

- [ ] **Step 1: Add the missing SDK decoder entry**

```cpp
{0x0702, "ERR_NO_SYS_INT_SIGNAL",
 "system interrupt is not running; verify EtherCAT is operational",
 ErrorAction::Ethercat},
```

- [ ] **Step 2: Insert the gate immediately after a successful `IMC_ScanCardEcat`**

```cpp
unsigned int masterStatus = 0;
Imc60gMasterInfo masterInfo;
const short requiredAxisCount = static_cast<short>(qMax(profile_.axisX, profile_.axisY) + 1);
if (!callSucceeded(api_->ethercatMasterStatus(0, &masterStatus),
        "IMC_GetEcatMasterSts", kNoAxis, errorMessage)
    || !callSucceeded(api_->ethercatMasterInfo(0, &masterInfo),
        "IMC_GetEcatMasterInfo", kNoAxis, errorMessage)) {
    goto fail;
}
if (masterStatus != kEthercatMasterOperational) {
    setError(errorMessage, QString("IMC60G EtherCAT master is not OP after automatic scan: status=%1 expected=6 (OP).")
        .arg(masterStatus));
    goto fail;
}
if (masterInfo.axisCount < requiredAxisCount) {
    setError(errorMessage, QString("IMC60G EtherCAT discovery found %1 axis resources; X/Y require physical axes 0 and 1.")
        .arg(masterInfo.axisCount));
    goto fail;
}
```

Place the gate before emergency configuration, axis-status clearing, and every Servo On call. `qMax` derives the required count from the locked profile rather than introducing a new constant.

- [ ] **Step 3: Run the focused test and observe green**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_qmake_test.ps1 -Project printing/tests/imc60g_motion_tests.pro
```

Expected: exit code `0`, with no `SAFETY FAIL` output.

- [ ] **Step 4: Commit the focused change**

```powershell
git add printing/IImc60gApi.h printing/Imc60gApi.h printing/Imc60gApi.cpp printing/Imc60gMotionController.cpp printing/tests/imc60g_safety_tests.cpp printing/tests/test_imc60g_motion.cpp printing/tests/test_sv660n_exposure.cpp
git commit -m "fix: gate IMC60G servo enablement on EtherCAT OP"
```

### Task 4: Verify complete printing integration

**Files:**

- Verify: `scripts/test_printing.ps1`

**Interfaces:**

- Consumes: all print-module tests, x64 release build, runtime staging checks.
- Produces: evidence that the adapter contract expansion does not break printing, exposure, UI, or deployment.

- [ ] **Step 1: Run the full verification script**

```powershell
powershell -ExecutionPolicy Bypass -File scripts\test_printing.ps1
```

Expected: exit code `0`, seven qmake test projects report `PASS`, then `mergeholo x64 build: PASS` and `runtime staging: PASS`.

- [ ] **Step 2: Verify the final patch scope**

```powershell
git diff --check HEAD; git status --short
```

Expected: no whitespace errors; only intended IMC60G adapter/controller/test files and the new plan are attributable to this task.

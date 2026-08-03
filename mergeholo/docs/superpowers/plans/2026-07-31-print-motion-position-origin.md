# Print Motion Position And Origin Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (- [ ]) syntax for tracking.

**Goal:** Display live X/Y position during a print and provide safe, session-only set-origin and return-to-origin controls for the IMC60G print axes.

**Architecture:** Imc60gMotionController owns planned-position reads and the safe two-axis coordinate reset. PrintController::Worker turns pulses into millimeters and samples on PrintJobRunner::frameAdvanced, which already runs on the IMC owner thread. The dialog renders existing position signals and sends origin commands through IPrintController.

**Tech Stack:** C++17, Qt 5.15 signals/slots, QElapsedTimer, IMC60G SDK adapter, qmake/nmake.

## Global Constraints

- Keep IMC SDK calls on the print worker. Do not create another polling thread.
- Read planned position with IMC_GetAxPrfPos32, matching V2's default position mode.
- Samples during printing are throttled to 10 Hz.
- Origin is session-only: never write it to an ini file.
- Origin commands are valid only in PrintUiState::Ready with no print ownership.
- Preserve existing EtherCAT startup and automatic cancellation cleanup.

---

### Task 1: Add Safe IMC60G Position And Origin Operations

**Files:**
- Modify: printing/Imc60gMotionController.h
- Modify: printing/Imc60gMotionController.cpp
- Test: printing/tests/test_imc60g_motion.cpp

**Interfaces:**
- Produces: bool readMappedPlannedPositions(int* xPulses, int* yPulses, QString* errorMessage = nullptr).
- Produces: bool setCurrentPositionAsLogicalOrigin(QString* errorMessage = nullptr).
- Consumes: mapped Axis1/X, Axis0/Y, plannedPosition, stop, setCurrentPosition, syncPosition, and status reads.

- [ ] **Step 1: Write the failing tests**

Extend RecordingImc60gApi to hold independently configured planned X/Y values and record stop, current-position, and sync calls. Add these tests:

~~~cpp
int xPulses = 0;
int yPulses = 0;
QString error;
expect(controller.readMappedPlannedPositions(&xPulses, &yPulses, &error),
    "mapped planned-position read must succeed: " + error);
expect(xPulses == 12000 && yPulses == -34000,
    "reader must preserve Axis1/X and Axis0/Y mapping");
~~~

Add a set-origin test that starts with both axes nonzero, confirms both are stopped and idle before rebasing, then asserts both positions are zero after the current-position/sync/reset sequence. Add a negative test that calls beginPrint() first and checks set-origin fails without touching SDK positions.

- [ ] **Step 2: Run the test to verify it fails**

Run:

~~~powershell
Set-Location C:/wzp/Holographicface/mergeholo/printing/tests
C:/wzp/QT/5.15.0/msvc2019_64/bin/qmake.exe imc60g_motion_tests.pro "CONFIG+=release"
nmake /f Makefile.Release
./release/imc60g_motion_tests.exe
~~~

Expected: compilation fails because the two motion-controller methods are absent.

- [ ] **Step 3: Implement the minimal motion operations**

Add both declarations. The reader locks mutex_, rejects null outputs and unavailable hardware, and makes exactly two mapped plannedPosition calls.

Set-origin locks mutex_, rejects any active print, issues stop commands for both mapped axes, then calls the private no-relock waitPrintAxisStopped helper for Y and X before resetting either coordinate. It then executes the same reset sequence already used by homing for X and Y:

~~~cpp
clearAxisStatus(card, axis);
setCurrentPosition(card, axis, 0.0);
syncPosition(card, axis);
setCurrentPosition(card, axis, 0.0);
clearAxisStatus(card, axis);
~~~

Use appendTaskError to preserve any per-axis error. Do not modify returnToLogicalZero, configuration, or physical-home state.

- [ ] **Step 4: Run the test to verify it passes**

Run the command from Step 2.

Expected: All IMC60G motion tests passed. with exit code 0.

- [ ] **Step 5: Commit**

~~~powershell
git add printing/Imc60gMotionController.h printing/Imc60gMotionController.cpp printing/tests/test_imc60g_motion.cpp
git commit -m "feat: add IMC logical origin operations"
~~~

### Task 2: Add Tested Position Sampling And Controller Routing

**Files:**
- Create: printing/PrintPositionSampler.h
- Create: printing/PrintPositionSampler.cpp
- Modify: printing/tests/test_v2_print_engine.cpp
- Modify: printing/tests/v2_print_engine_tests.pro
- Modify: mergeholo.pro
- Modify: printing/PrintController.h
- Modify: printing/PrintController.cpp

**Interfaces:**
- Produces: PrintPositionSampler::isDue(qint64 nowMs) and PrintPositionSampler::toMillimeters(int xPulses, int yPulses, const PrintAxisConfig& x, const PrintAxisConfig& y, QPointF* result).
- Produces: IPrintController::setLogicalOrigin() and IPrintController::returnToLogicalOrigin() slots.
- Consumes: readMappedPlannedPositions, setCurrentPositionAsLogicalOrigin, returnToLogicalZero, and verifyLogicalZero.
- Produces: the existing positionsChanged(double xMillimeters, double yMillimeters) signal.

- [ ] **Step 1: Write the failing sampler tests**

Add the new source to v2_print_engine_tests.pro and focused test functions to test_v2_print_engine.cpp with no hardware dependency. Cover first-sample behavior, the 100 ms boundary, position conversion, negative coordinates, and invalid scale rejection:

~~~cpp
PrintPositionSampler sampler(100);
expect(sampler.isDue(0), "first printing position sample is due");
expect(!sampler.isDue(99), "sample must be throttled below 100 ms");
expect(sampler.isDue(100), "sample is due at the 100 ms boundary");

QPointF millimeters;
expect(PrintPositionSampler::toMillimeters(12000, -34000, xAxis, yAxis, &millimeters),
    "valid pulse scales must convert");
expect(millimeters == QPointF(6.0, -68.0), "pulse conversion must preserve sign");
~~~

- [ ] **Step 2: Run the test to verify it fails**

Run:

~~~powershell
Set-Location C:/wzp/Holographicface/mergeholo/printing/tests
C:/wzp/QT/5.15.0/msvc2019_64/bin/qmake.exe v2_print_engine_tests.pro "CONFIG+=release"
nmake /f Makefile.Release
./release/v2_print_engine_tests.exe
~~~

Expected: compilation fails because PrintPositionSampler does not exist.

- [ ] **Step 3: Implement the sampler and controller routing**

Implement the sampler as a small pure module. isDue() records the first and each due timestamp; toMillimeters() divides X and Y independently by subdivision multiplied by resolution and returns false for a non-positive scale.

Add the two slots to IPrintController and PrintController, queueing both to Worker as existing manual-motion methods do. The worker owns one QElapsedTimer and one PrintPositionSampler. Refactor the ready-state pollPositions() around this helper:

~~~cpp
void publishPositions()
{
    int xPulses = 0;
    int yPulses = 0;
    QString detail;
    if (!motion_.readMappedPlannedPositions(&xPulses, &yPulses, &detail)) {
        fail(detail);
        return;
    }
    QPointF position;
    if (!PrintPositionSampler::toMillimeters(xPulses, yPulses,
            config_.axisX, config_.axisY, &position)) {
        fail("IMC60G position scale is invalid.");
        return;
    }
    post([position](PrintController* owner) {
        emit owner->positionsChanged(position.x(), position.y());
    });
}
~~~

Connect PrintJobRunner::frameAdvanced directly to a worker callback. When state is Printing and sampler.isDue(elapsedTimer.elapsed()) is true, call publishPositions(). Do not use the ready-state timer or a second thread. The callback must not be blocked by busy_, because the runner legitimately owns that flag during print execution.

Implement set-origin only for ready/not-busy worker state. On success emit zero coordinates and status; on failure use fail(). Implement return-origin with active config_.axisX/Y, returnToLogicalZero, then verifyLogicalZero, followed by publishPositions(). Failures use fail().

- [ ] **Step 4: Run the test to verify it passes**

Run the command from Step 2.

Expected: v2_print_engine_tests exits with code 0, including the sampler tests.

- [ ] **Step 5: Commit**

~~~powershell
git add mergeholo.pro printing/PrintPositionSampler.h printing/PrintPositionSampler.cpp printing/tests/test_v2_print_engine.cpp printing/tests/v2_print_engine_tests.pro printing/PrintController.h printing/PrintController.cpp
git commit -m "feat: publish print positions and origin commands"
~~~

### Task 3: Add And Gate Print Dialog Origin Controls

**Files:**
- Modify: ui/Print9030Dialog.ui
- Modify: widgets/Print9030Dialog.cpp
- Test: widgets/tests/test_print9030_dialog.cpp

**Interfaces:**
- Consumes: setLogicalOrigin(), returnToLogicalOrigin(), positionsChanged, and PrintUiState.
- Produces: Ready-only setOriginButton and returnOriginButton.

- [ ] **Step 1: Write the failing dialog test**

Extend RecordingPrintController with the two slots and a publishPositions(double, double) helper. Add:

~~~cpp
controller.publishState(PrintUiState::Ready);
button(dialog, "setOriginButton")->click();
button(dialog, "returnOriginButton")->click();
expect(controller.commands == QStringList({"setLogicalOrigin", "returnToLogicalOrigin"}),
    "origin controls must dispatch exactly once");

controller.publishState(PrintUiState::Printing);
expect(!button(dialog, "setOriginButton")->isEnabled()
        && !button(dialog, "returnOriginButton")->isEnabled(),
    "origin controls must be locked during printing");

controller.publishPositions(1.234, -5.678);
expect(xLabel->text() == "X: 1.234 mm" && yLabel->text() == "Y: -5.678 mm",
    "position signals must render three decimal millimeter values");
~~~

- [ ] **Step 2: Run the test to verify it fails**

Run:

~~~powershell
$env:QT_QPA_PLATFORM = "offscreen"
$env:PATH = "C:/wzp/QT/5.15.0/msvc2019_64/bin;C:/wzp/Holographicface/opencv450/opencv/build/x64/vc15/bin;" + $env:PATH
Set-Location C:/wzp/Holographicface/mergeholo/widgets/tests
C:/wzp/QT/5.15.0/msvc2019_64/bin/qmake.exe print9030_dialog_tests.pro "CONFIG+=release"
nmake /f Makefile.Release
./release/print9030_dialog_tests.exe
~~~

Expected: compilation fails because the origin controls and controller slots do not exist.

- [ ] **Step 3: Implement the dialog controls**

Add command buttons named setOriginButton (设置原点) and returnOriginButton (归零) after manualStopButton in manualMotionLayout. Connect each once in the constructor. In applyState, enable both only when ready is true. Keep existing X/Y label formatting and avoid confirmation dialogs, persistent origin fields, or non-print axes.

- [ ] **Step 4: Run the test to verify it passes**

Run the command from Step 2.

Expected: print 9030 dialog tests passed with exit code 0.

- [ ] **Step 5: Commit**

~~~powershell
git add ui/Print9030Dialog.ui widgets/Print9030Dialog.cpp widgets/tests/test_print9030_dialog.cpp
git commit -m "feat: add print origin controls"
~~~

### Task 4: Regression And Hardware Verification

**Files:**
- No production-file changes expected.

- [ ] **Step 1: Run focused regression tests**

Run the IMC motion, V2 engine (including the sampler tests), and dialog test executables. Expected: each exits with code 0.

- [ ] **Step 2: Check the owned diff**

Run:

~~~powershell
Set-Location C:/wzp/Holographicface/mergeholo
git diff --check -- mergeholo.pro printing/Imc60gMotionController.h printing/Imc60gMotionController.cpp printing/PrintController.h printing/PrintController.cpp printing/PrintPositionSampler.h printing/PrintPositionSampler.cpp printing/tests ui/Print9030Dialog.ui widgets/Print9030Dialog.cpp widgets/tests/test_print9030_dialog.cpp
~~~

Expected: no whitespace errors in feature-owned files.

- [ ] **Step 3: Perform hardware acceptance**

With supervised hardware: connect/home, manually move X/Y, set origin, confirm both labels read 0.000 mm; move again, return to origin, and verify both axes arrive at the set origin. Start a short print and confirm both labels update during Y scan. Cancel and confirm existing automatic return-to-logical-zero remains intact.

- [ ] **Step 4: Commit verification-only test adjustments**

~~~powershell
git add printing/tests widgets/tests
git commit -m "test: cover print position and origin workflow"
~~~

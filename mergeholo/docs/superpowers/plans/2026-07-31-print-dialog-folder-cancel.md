# Print Dialog Folder Loading and Cancellation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep the print dialog responsive while loading folders, infer valid `RRRCCC` grids, and make verified cancellation ready for the next print without weakening motion safety.

**Architecture:** `PrintImageSource` owns deterministic folder ordering and optional grid metadata. `Print9030Dialog` owns the asynchronous operation and applies one finished result on the GUI thread. `PrintJobRunner`, `PrintController`, and `Imc60gMotionController` retain safety ownership: cancel can reset UI progress, but only verified zero return reaches `Ready`.

**Tech Stack:** Qt 5.15 Widgets, Qt Concurrent, C++17, QFutureWatcher, qmake, IMC60G SDK adapter.

## Global Constraints

- Preserve all current `Print9030Dialog.ui` object names and existing configuration keys.
- `001001.jpg` to `100100.jpg` must infer a 100 by 100 row-major grid.
- Non-`RRRCCC` folders must remain usable with manually configured dimensions.
- Cancellation may clear progress only; Start is enabled solely after verified cleanup reaches `Ready`.
- Do not stage or revert unrelated working-tree changes.

---

### Task 1: Deterministic Folder Grid Metadata

**Files:**
- Modify: `printing/PrintImageSource.h`
- Modify: `printing/PrintImageSource.cpp`
- Modify: `widgets/tests/test_print9030_dialog.cpp`

**Interfaces:**
- Produces `PrintImageFolderLoadResult loadPrintImagesFromFolderWithGridInfo(const QString&, QString*)`.
- `PrintImageFolderLoadResult` contains `PrintImageSet images`, `int gridRows`, `int gridColumns`, and `QString gridWarning`.
- `bool hasInferredGrid() const` is true only when both dimensions are positive.
- The existing `loadPrintImagesFromFolder` delegates to the new function and returns its `images` member.

- [ ] **Step 1: Write the failing source tests**

Add a test that writes a 2 by 3 fixture with intentionally non-lexical creation order and a second fixture with ordinary names:

```cpp
void testFolderGridMetadataOrdersRrrCccFrames()
{
    QTemporaryDir root;
    const QImage image(1, 1, QImage::Format_RGB32);
    for (const QString& name : {"002003.jpg", "001002.jpg", "002001.jpg",
             "001001.jpg", "002002.jpg", "001003.jpg"}) {
        expect(image.save(QDir(root.path()).filePath(name)), "fixture image must save");
    }
    QString error;
    const PrintImageFolderLoadResult result =
        loadPrintImagesFromFolderWithGridInfo(root.path(), &error);
    expect(error.isEmpty() && result.images.isValid(), "RRRCCC folder must load");
    expect(result.hasInferredGrid() && result.gridRows == 2 && result.gridColumns == 3,
        "RRRCCC folder must infer dimensions");
    expect(result.gridWarning.isEmpty(), "complete RRRCCC grid must not warn");
}

void testFolderGridMetadataLeavesManualDimensionsForOrdinaryNames()
{
    QTemporaryDir root;
    QImage image(1, 1, QImage::Format_RGB32);
    expect(image.save(QDir(root.path()).filePath("front.jpg")), "fixture image must save");
    expect(image.save(QDir(root.path()).filePath("back.jpg")), "fixture image must save");
    QString error;
    const PrintImageFolderLoadResult result =
        loadPrintImagesFromFolderWithGridInfo(root.path(), &error);
    expect(error.isEmpty() && result.images.isValid(), "ordinary folder must load");
    expect(!result.hasInferredGrid() && !result.gridWarning.isEmpty(),
        "ordinary names must retain manual grid dimensions");
}
```

- [ ] **Step 2: Run the dialog test to verify the tests fail**

Run:

```powershell
$qmake = 'C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe'
& $qmake widgets\tests\print9030_dialog_tests.pro -o widgets\tests\Makefile
nmake /f widgets\tests\Makefile.Release
& widgets\tests\release\print9030_dialog_tests.exe
```

Expected: compilation failure because `PrintImageFolderLoadResult` and `loadPrintImagesFromFolderWithGridInfo` do not exist.

- [ ] **Step 3: Implement the metadata loader**

Declare the value type in `printing/PrintImageSource.h`:

```cpp
struct PrintImageFolderLoadResult {
    PrintImageSet images;
    int gridRows = 0;
    int gridColumns = 0;
    QString gridWarning;

    bool hasInferredGrid() const { return gridRows > 0 && gridColumns > 0; }
};

PrintImageFolderLoadResult loadPrintImagesFromFolderWithGridInfo(
    const QString& folderPath, QString* errorMessage = nullptr);
```

In `printing/PrintImageSource.cpp`, enumerate files once, parse basenames with
`QRegularExpression("^(\\d{3})(\\d{3})$")`, and retain each file's row and
column. Accept inferred dimensions only when every file matches, row and
column numbers are at least one, all cells are unique, rows and columns begin
at one, and the file count equals `maxRow * maxColumn`. Sort accepted entries
by `(row, column)` before calling `decodeFrame`. For all other folders, sort
by the current case-insensitive filename order and set:

```cpp
result.gridWarning = QStringLiteral(
    "Image names do not form a complete RRRCCC grid; keep manual row and column values.");
```

Return image decode and directory errors through `errorMessage`; do not turn a
non-inferable but readable folder into an error. Keep the legacy function as:

```cpp
PrintImageSet loadPrintImagesFromFolder(const QString& folderPath, QString* errorMessage)
{
    return loadPrintImagesFromFolderWithGridInfo(folderPath, errorMessage).images;
}
```

- [ ] **Step 4: Run the source tests to verify they pass**

Run the command from Step 2.

Expected: `print9030_dialog_tests.exe` exits `0` and the original single-image
folder test still passes.

- [ ] **Step 5: Commit the focused source change**

```powershell
git add printing/PrintImageSource.h printing/PrintImageSource.cpp widgets/tests/test_print9030_dialog.cpp
git commit -m "feat: infer print folder grid dimensions"
```

### Task 2: Responsive Dialog Folder Loading and Parameter Alignment

**Files:**
- Modify: `widgets/Print9030Dialog.h`
- Modify: `widgets/Print9030Dialog.cpp`
- Modify: `ui/Print9030Dialog.ui`
- Modify: `mergeholo.pro`
- Modify: `widgets/tests/print9030_dialog_tests.pro`
- Modify: `widgets/tests/test_print9030_dialog.cpp`

**Interfaces:**
- `setManualImageFolder(const QString&)` starts an asynchronous request and returns without decoding on the caller thread.
- Private `FolderLoadCompletion`, `beginFolderLoad(const QString&)`, `applyFolderLoadResult()`, and `folderLoading_` coordinate the UI state.
- `applyState` prevents Start, Preview, browse, path editing, and source selection while `folderLoading_` is true.

- [ ] **Step 1: Write failing dialog tests**

Extend `RecordingPrintController` with a `publishProgress` helper and add a
bounded event-loop helper:

```cpp
void publishProgress(int value, const QString& detail)
{
    emit progressChanged(value, detail);
}

bool waitUntil(const std::function<bool()>& predicate, int timeoutMs = 5000)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
        QThread::msleep(1);
    }
    return predicate();
}
```

Test asynchronous loading and geometry:

```cpp
dialog.setManualImageFolder(root.path());
expect(!button(dialog, "startButton")->isEnabled(), "Start is disabled while folder loads");
expect(!button(dialog, "previewButton")->isEnabled(), "Preview is disabled while folder loads");
expect(waitUntil([&] { return button(dialog, "previewButton")->isEnabled(); }),
    "folder load must complete without blocking the event loop");
expect(dialog.findChild<QSpinBox*>("gridRowsSpin")->value() == 2,
    "recognized rows must populate the dialog");
expect(dialog.findChild<QSpinBox*>("gridColumnsSpin")->value() == 3,
    "recognized columns must populate the dialog");
```

Also assert paired layout geometry after `dialog.show()` and processing events:

```cpp
expect(rowSpacing->geometry().x() < columnSpacing->geometry().x()
        && rowSpacing->geometry().y() == columnSpacing->geometry().y(),
    "spacing editors must share one row");
expect(rows->geometry().y() == columns->geometry().y(),
    "grid count editors must share one row");
```

- [ ] **Step 2: Run the dialog test to verify the asynchronous assertions fail**

Run the command from Task 1, Step 2.

Expected: the existing synchronous `setManualImageFolder` either enables Start
immediately or lacks the requested inferred dimensions, so the new assertions fail.

- [ ] **Step 3: Implement asynchronous ownership and the aligned grid**

Add `concurrent` to both `.pro` files:

```pro
QT += core gui widgets concurrent
```

Give `Print9030Dialog` this private completion value type, a
`QFutureWatcher<FolderLoadCompletion>`, a monotonic
`quint64 folderLoadRequestId_`, and `bool folderLoading_`:

```cpp
struct FolderLoadCompletion {
    PrintImageFolderLoadResult source;
    QString errorMessage;
    quint64 requestId = 0;
};
```

In the constructor connect `QFutureWatcher::finished` to a GUI-thread method.
Start work with the request id captured by value:

```cpp
const quint64 requestId = ++folderLoadRequestId_;
folderLoading_ = true;
applyState(*stateStorage_);
folderLoadWatcher_.setFuture(QtConcurrent::run([folderPath, requestId] {
    QString error;
    FolderLoadCompletion completion;
    completion.source = loadPrintImagesFromFolderWithGridInfo(folderPath, &error);
    completion.errorMessage = error;
    completion.requestId = requestId;
    return completion;
}));
```

Only apply a completion whose id equals `folderLoadRequestId_`.
On success assign `folderImages_`, select the folder source, populate
`gridRowsSpin` and `gridColumnsSpin` only when `hasInferredGrid()` is true,
and display `gridWarning` only when no load error exists. On error clear the
folder image set and show the error. In all completion paths clear
`folderLoading_`, refresh the source summary, and call `applyState`.

Rearrange `mainGrid` in `ui/Print9030Dialog.ui` into columns 0-3 and rows 0-3:

```text
row 0: rowSpacingLabel, rowSpacingSpin, columnSpacingLabel, columnSpacingSpin
row 1: gridRowsLabel, gridRowsSpin, gridColumnsLabel, gridColumnsSpin
row 2: widthScaleLabel, widthScaleSpin, heightScaleLabel, heightScaleSpin
row 3: addTempPulseLabel, addTempPulseSpin, leadPulseLabel, leadPulseSpin
```

- [ ] **Step 4: Run the dialog test to verify it passes**

Run the command from Task 1, Step 2.

Expected: `print9030_dialog_tests.exe` exits `0`, including the inferred 2 by
3 source, loading lock, preview, and layout geometry tests.

- [ ] **Step 5: Commit the dialog behavior**

```powershell
git add mergeholo.pro widgets/Print9030Dialog.h widgets/Print9030Dialog.cpp ui/Print9030Dialog.ui widgets/tests/print9030_dialog_tests.pro widgets/tests/test_print9030_dialog.cpp
git commit -m "fix: load print folders without blocking dialog"
```

### Task 3: Cancellation Progress and Ready-State Recovery

**Files:**
- Modify: `printing/PrintJobRunner.cpp`
- Modify: `printing/PrintController.cpp`
- Modify: `printing/tests/test_v2_print_engine.cpp`

**Interfaces:**
- A cancellation after `finishAndCleanup` emits `progressChanged(0, ...)`.
- A cancellation whose runner state is `Ready` clears `errorChanged`; faulted cleanup continues to publish its diagnostic.

- [ ] **Step 1: Write the failing cancellation progress test**

Extend `testCancelDuringPresentDoesNotAdvanceAnotherVBlank` with a progress
capture, then assert the final cancellation event is zero:

```cpp
QVector<int> progress;
QObject::connect(&runner, &PrintJobRunner::progressChanged,
    [&progress](int value, const QString&) { progress.append(value); });
// Existing start/cancel exercise remains unchanged.
expect(!progress.isEmpty() && progress.last() == 0,
    "completed cancellation cleanup must reset progress to zero");
expect(runner.state() == PrintJobState::Ready,
    "verified cancellation cleanup must restore Ready state");
```

Add the same final-zero assertion to the failed-cleanup cancellation fixture so
the visible progress is reset even though its final state remains `Fault`.

- [ ] **Step 2: Run the engine test to verify the assertion fails**

Run:

```powershell
$qmake = 'C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe'
& $qmake printing\tests\v2_print_engine_tests.pro -o printing\tests\Makefile
nmake /f printing\tests\Makefile.Release
& printing\tests\release\v2_print_engine_tests.exe
```

Expected: the new assertion fails because the runner does not emit a terminal
zero progress update.

- [ ] **Step 3: Emit terminal cancellation progress and clear only safe cancel diagnostics**

At the end of `PrintJobRunner::finishAndCleanup`, before `finished`, add:

```cpp
const bool cancellationFinished = !success
    && cancelRequested_.load(std::memory_order_acquire);
if (cancellationFinished) {
    emit progressChanged(0, safe
            ? QStringLiteral("Print job cancelled; cleanup verified.")
            : QStringLiteral("Print job cancelled; cleanup requires attention."));
}
```

In the `PrintController::Worker` `finished` connection, capture the runner
state before posting. Send an empty error only for a cancelled runner that is
already `Ready`:

```cpp
const bool safelyCancelled = !success && runner_->state() == PrintJobState::Ready;
post([success, detail, safelyCancelled](PrintController* owner) {
    emit owner->statusChanged(safelyCancelled
            ? QStringLiteral("Print job cancelled; ready for the next job.") : detail);
    if (safelyCancelled) emit owner->errorChanged(QString());
    else if (!success) emit owner->errorChanged(detail);
    emit owner->safeStopCompleted();
});
```

Do not change `finishAndCleanup`'s fault decision or `applyState`'s Ready gate.

- [ ] **Step 4: Run the engine test to verify it passes**

Run the command from Step 2.

Expected: `v2_print_engine_tests.exe` exits `0`; cleanup failure tests still
assert `Fault`, while successful cancellation finishes at zero progress and `Ready`.

- [ ] **Step 5: Commit cancellation lifecycle behavior**

```powershell
git add printing/PrintJobRunner.cpp printing/PrintController.cpp printing/tests/test_v2_print_engine.cpp
git commit -m "fix: reset print progress after cancellation"
```

### Task 4: Axis-Specific Return-to-Zero Timeout

**Files:**
- Modify: `printing/Imc60gMotionController.cpp`
- Modify: `printing/tests/test_imc60g_motion.cpp`

**Interfaces:**
- Private implementation helper `int returnToZeroTimeoutMs(int plannedPosition, const PrintAxisConfig&, int minimumTimeoutMs)` returns a bounded timeout for one axis.
- `returnToLogicalZero` reads each axis's planned position before the zero move and waits using that axis's computed timeout.

- [ ] **Step 1: Write the failing slow-X return test**

Extend the existing `RecordingImc60gApi` to retain `QHash<short, int>
plannedPositions`, add `QHash<short, int> returningBusyPolls`, and make
`axisStatus` OR `kBusy` while a returning axis still has busy polls remaining.
Add a test-local advancing clock:

```cpp
class AdvancingClock final : public IImc60gClock {
public:
    qint64 nowMs() const override { return nowMs_; }
    void sleepMs(int milliseconds) override { nowMs_ += milliseconds; }
private:
    qint64 nowMs_ = 0;
};

void testSlowXReturnUsesItsOwnMotionBudget()
{
    RecordingImc60gApi api;
    AdvancingClock clock;
    api.plannedPositions.insert(1, 149000);
    api.returningBusyPolls.insert(1, 15000);
    Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
    QString error;
    expect(controller.connectAndHome(&error), "fixture must connect: " + error);
    expect(controller.beginPrint(&error), "fixture must acquire print ownership: " + error);
    Print9030Config config = defaultPrint9030Config();
    config.axisX.speedOfMovement = 5000;
    config.axisX.acceleratedVelocity = 50000;
    expect(controller.returnToLogicalZero(config.axisX, config.axisY, 9666, &error),
        "slow X zero return must outlive the Y-derived legacy timeout: " + error);
}
```

- [ ] **Step 2: Run the IMC test to verify it fails at the legacy timeout**

Run:

```powershell
$qmake = 'C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe'
& $qmake printing\tests\imc60g_motion_tests.pro -o printing\tests\Makefile
nmake /f printing\tests\Makefile.Release
& printing\tests\release\imc60g_motion_tests.exe
```

Expected: the test fails with `axis=1 timeoutMs=9666`.

- [ ] **Step 3: Compute the return timeout per axis and use it**

Add a file-local helper in `Imc60gMotionController.cpp`:

```cpp
int returnToZeroTimeoutMs(int plannedPosition, const PrintAxisConfig& config,
    int minimumTimeoutMs)
{
    const qint64 distance = std::llabs(static_cast<qint64>(plannedPosition));
    const qint64 speed = qMax<qint64>(1, config.speedOfMovement);
    const qint64 acceleration = qMax<qint64>(1, config.acceleratedVelocity);
    const qint64 cruiseMs = (distance * 1000 + speed - 1) / speed;
    const qint64 rampMs = (2 * speed * 1000 + acceleration - 1) / acceleration;
    return static_cast<int>(qBound<qint64>(5000,
        qMax<qint64>(minimumTimeoutMs, cruiseMs + rampMs + 5000), 300000));
}
```

In `returnToLogicalZero`, obtain `plannedPosition` for X and Y before each
`startPrintMove`; on an unreadable position use the configured full-axis travel
(`maxDistance * subdivision * resolution`) to calculate a conservative budget.
Pass the resulting individual budget to `waitPrintAxisStopped`. In the fake,
make `startPtp(axis, 0)` begin the configured busy-poll sequence. Preserve all
SDK error reporting and retain the sequential X then Y return order.

- [ ] **Step 4: Run the IMC test to verify it passes**

Run the command from Step 2.

Expected: `imc60g_motion_tests.exe` exits `0`; existing alarm and stuck-axis
tests still fail safely when the physical busy bit does not clear before the
bounded 300000 ms limit.

- [ ] **Step 5: Commit the motion-timeout fix**

```powershell
git add printing/Imc60gMotionController.cpp printing/tests/test_imc60g_motion.cpp
git commit -m "fix: budget IMC zero return per axis"
```

### Task 5: Integration Verification and Release Build

**Files:**
- Modify: none unless a focused test exposes a defect in Tasks 1-4.

**Interfaces:**
- Consumes the four committed deliverables.
- Produces a verified Release executable at `00-bin/mergeholo.exe`.

- [ ] **Step 1: Run all focused test executables**

```powershell
& widgets\tests\release\print9030_dialog_tests.exe
& printing\tests\release\v2_print_engine_tests.exe
& printing\tests\release\imc60g_motion_tests.exe
```

Expected: each executable exits `0`.

- [ ] **Step 2: Build Release with the production SDK paths**

```powershell
$env:OPENCV_ROOT = 'C:\wzp\Holographicface\opencv450\opencv\build'
$env:JP_LF_V4_ROOT = 'C:\wzp\Holographicface\holocamera\HoloTest\Holo_v4.1.1'
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Config release -QtRoot 'C:\wzp\QT\5.15.0\msvc2019_64'
```

Expected: build exits `0` and creates `00-bin/mergeholo.exe`.

- [ ] **Step 3: Inspect the final diff and commit integration metadata only when needed**

```powershell
git diff --check
git status --short
git log --oneline -5
```

Expected: no whitespace errors; only the intended files from Tasks 1-4 are
attributable to this work, and unrelated pre-existing changes remain untouched.

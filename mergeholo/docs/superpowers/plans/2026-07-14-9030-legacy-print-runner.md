# 9030 Legacy Print Runner Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `PrintJobRunner` execute the legacy 9030 D3D11/VBlank presentation and real-card serpentine scan sequence, with no simulation fallback and safe X/Y return-to-zero cleanup.

**Architecture:** Separate image decoding, Windows second-screen presentation, motion-card adaptation, and pure legacy timing calculations. `PrintJobRunner` owns the worker-thread orchestration while injected motion and frame-presenter interfaces allow the timing and error paths to be tested without a connected card or external display.

**Tech Stack:** Qt 5.15 / C++17 / qmake, Windows monitor APIs, Direct3D 11, DXGI, DfjzhControlerDll dynamic exports, Qt console tests.

## Global Constraints

- Target Windows x64 with Qt 5.15.0 MSVC2019; do not introduce MinGW-only or Qt 6 APIs.
- Preserve legacy external-screen selection: choose the final non-primary monitor from `EnumDisplayMonitors` enumeration.
- Require D3D11 frame presentation and `IDXGIOutput::WaitForVBlank`; no primary-screen or `DwmFlush` fallback.
- Use board 0 and the exact legacy card calls listed in the approved design.
- `InitCard_ID`, DLL loading, or export resolution failures must fail the job; never enter simulation.
- Use the active legacy timing formula, `addTempPulse`, `leadPulse`, 60 Hz frame-fit validation, and row serpentine order.
- On completion, cancellation, or any runtime failure: disarm O1, force O1 low, stop axes, move X/Y to logical zero, wait for both axes, then exit the card.
- Keep all existing user changes intact; stage only files changed by the current task.

---

## File Structure

| File | Responsibility |
| --- | --- |
| `printing/PrintFrame.h` | Immutable decoded BGR/BGRA frame passed from image sources to presenters. |
| `printing/PrintImageSource.{h,cpp}` | Convert elemental-memory or folder images into `PrintFrame` values. |
| `printing/IMotionController.h` | Card operations required by the legacy scan: initialize, logical zero, absolute move, wait, stop, O1 control. |
| `printing/DfjzhMotionController.{h,cpp}` | Dynamic implementation of the Dfjzh legacy DLL API with strict failures. |
| `printing/IPrintFramePresenter.h` | Testable synchronous display/VBlank boundary used by the runner. |
| `printing/LegacyD3DImageRenderer.{h,cpp}` | Direct port of the legacy D3D11 swap-chain renderer using `PrintFrame`. |
| `printing/LegacySecondScreenPresenter.{h,cpp}` | Legacy monitor selection, Qt-owned window, GUI-thread dispatch, and VBlank access. |
| `printing/LegacyPrintTiming.{h,cpp}` | Pure validation and pulse/row-coordinate calculations from the active legacy scan loop. |
| `printing/PrintJobRunner.{h,cpp}` | Worker-thread orchestration of preflight, card actions, scan, progress, cancellation, and cleanup. |
| `printing/tests/test_printing_modules.cpp` | Console tests for image frames, card failures, timing, runner order, cancellation, and cleanup. |
| `printing/tests/printing_tests.pro` | Adds Qt GUI support and runner sources to the console test executable. |
| `widgets/Print9030Dialog.{h,cpp}` | Creates the GUI-thread presenter and retains it for the lifetime of a print job. |
| `mergeholo.pro` | Builds all new printing sources and headers in the Windows Qt application. |

## Interfaces

```cpp
// printing/PrintFrame.h
enum class PrintPixelFormat { Bgr24, Bgra32 };

struct PrintFrame {
    QByteArray pixels;
    int width = 0;
    int height = 0;
    PrintPixelFormat format = PrintPixelFormat::Bgr24;

    bool isValid() const;
};

// printing/IMotionController.h
class IMotionController {
public:
    virtual ~IMotionController() = default;
    virtual bool initialize(QString* errorMessage = nullptr) = 0;
    virtual void shutdown() = 0;
    virtual bool setCurrentPositionAsOrigin(int axis, QString* errorMessage = nullptr) = 0;
    virtual bool moveTo(int axis, long targetPulse, const PrintAxisConfig& config,
                        QString* errorMessage = nullptr) = 0;
    virtual bool stopAxis(int axis, QString* errorMessage = nullptr) = 0;
    virtual bool waitUntilStopped(int axis, int timeoutMs,
                                  const std::atomic_bool& cancelRequested,
                                  QString* errorMessage = nullptr) = 0;
    virtual long readPosition(int axis) const = 0;
    virtual bool setExposureOutput(bool enabled, int outputIndex,
                                   QString* errorMessage = nullptr) = 0;
    virtual bool armExposureWindow(long beginPos, long endPos,
                                   QString* errorMessage = nullptr) = 0;
    virtual void disarmExposureWindow() = 0;
};

// printing/IPrintFramePresenter.h
class IPrintFramePresenter {
public:
    virtual ~IPrintFramePresenter() = default;
    virtual bool prepare(const PrintFrame& firstFrame, QSize targetSize,
                         QString* errorMessage = nullptr) = 0;
    virtual bool present(const PrintFrame& frame, QSize targetSize,
                         QString* errorMessage = nullptr) = 0;
    virtual bool waitForVBlank(QString* errorMessage = nullptr) = 0;
    virtual void shutdown() = 0;
};

// printing/LegacyPrintTiming.h
struct LegacyPrintTiming {
    long yStepPulse = 0;
    long xStepPulse = 0;
    long exposurePulse = 0;
    long accelerationPulse = 0;
    long totalPulse = 0;
    int framesPerImage = 0;
};

struct LegacyRowPlan {
    bool reverseY = false;
    long yTarget = 0;
    long yConstBegin = 0;
    long yConstEnd = 0;
    long yExposeBegin = 0;
    long compareBegin = 0;
    long compareEnd = 0;
};

bool calculateLegacyPrintTiming(const Print9030Config&, LegacyPrintTiming*, QString*);
LegacyRowPlan makeLegacyRowPlan(const LegacyPrintTiming&, const Print9030Config&,
                                long yStart, bool reverseY);
```

### Task 1: Decode Print Frames

**Files:**
- Create: `printing/PrintFrame.h`
- Modify: `printing/PrintImageSource.h`
- Modify: `printing/PrintImageSource.cpp`
- Modify: `printing/tests/test_printing_modules.cpp`
- Modify: `printing/tests/printing_tests.pro`

**Consumes:** `PrintImageSet`, `ElementalMemoryResult`.

**Produces:** `bool PrintImageSet::copyFrame(size_t index, PrintFrame* destination, QString* errorMessage) const`.

- [ ] **Step 1: Write the failing frame-decoding tests**

Add a memory-frame test and replace the empty folder files with a valid 2x1 image:

```cpp
void testMemoryFramePreservesBgrPixels()
{
    ElementalMemoryResult memory = makeMemoryResult();
    PrintImageSet set = makePrintImageSetFromElementalMemory(memory);
    PrintFrame frame;
    QString error;
    expect(set.copyFrame(1, &frame, &error), "memory frame should decode");
    expect(frame.width == 3 && frame.height == 2, "memory dimensions should match");
    expect(frame.format == PrintPixelFormat::Bgr24, "memory format should be BGR24");
    expect(frame.pixels.size() == static_cast<int>(memory.imageBytes), "frame byte count should match");
    expect(frame.pixels.at(0) == static_cast<char>(memory.imageBytes), "frame should use requested image offset");
}
```

Create the folder fixture with `QImage image(2, 1, QImage::Format_RGB32); image.fill(qRgb(0x11, 0x22, 0x33)); image.save(path);` and assert that `copyFrame` returns a valid `Bgr24` frame with six bytes.

- [ ] **Step 2: Run the test to verify it fails**

Run:

```powershell
$buildDir = "$PWD\FF-tmp\printing-tests"
New-Item -ItemType Directory -Force $buildDir | Out-Null
Push-Location $buildDir
cmd /d /c 'call "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && "C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe" "..\..\printing\tests\printing_tests.pro" "CONFIG+=release" -o Makefile && nmake /NOLOGO /f Makefile && release\printing_tests.exe'
Pop-Location
```

Expected: compilation fails because `PrintFrame` and `copyFrame` do not exist.

- [ ] **Step 3: Implement the minimal frame contract and decoding**

Define validity without accepting malformed byte counts:

```cpp
bool PrintFrame::isValid() const
{
    const int channels = format == PrintPixelFormat::Bgr24 ? 3 : 4;
    return width > 0 && height > 0
        && pixels.size() == width * height * channels;
}
```

For elemental memory, copy exactly `imageBytes` from `copyImage` and require it
to equal `rows * cols * 3`. For a folder file, use `QImageReader`, reject a null
image, convert to `QImage::Format_BGR888`, then deep-copy `sizeInBytes()` into
`PrintFrame::pixels`. Add `QT += gui` to the test project.

- [ ] **Step 4: Run the test to verify it passes**

Run the command from Step 2.

Expected: `printing module tests passed`.

- [ ] **Step 5: Commit the frame boundary**

```powershell
git add printing/PrintFrame.h printing/PrintImageSource.h printing/PrintImageSource.cpp printing/tests/test_printing_modules.cpp printing/tests/printing_tests.pro
git commit -m "feat: decode 9030 print frames"
```

### Task 2: Make the Motion Adapter Strict and Legacy-Compatible

**Files:**
- Modify: `printing/IMotionController.h`
- Modify: `printing/DfjzhMotionController.h`
- Modify: `printing/DfjzhMotionController.cpp`
- Modify: `printing/tests/test_printing_modules.cpp`
- Modify: `printing/tests/printing_tests.pro`

**Consumes:** `PrintAxisConfig`, board-0 `DfjzhControlerDll.dll` exports.

**Produces:** strict `initialize`, `setCurrentPositionAsOrigin`, and `moveTo` operations.

- [ ] **Step 1: Write the failing missing-DLL test**

Compile `DfjzhMotionController.cpp` into the console test target and add:

```cpp
void testMotionControllerRejectsMissingLibrary()
{
    DfjzhMotionController controller(QDir::temp().filePath("no-such-9030-card.dll"));
    QString error;
    expect(!controller.initialize(&error), "missing controller DLL must fail initialization");
    expect(error.contains("DLL"), "missing DLL error should identify the controller runtime");
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run the Task 1 test command.

Expected: assertion fails because current initialization returns success in simulation mode.

- [ ] **Step 3: Replace relative/simulation behavior with legacy absolute moves**

Change the interface to `setCurrentPositionAsOrigin` and `moveTo` as declared
above. In `DfjzhMotionController`, resolve `Home_ID` with the signature
`short(__stdcall*)(unsigned short, unsigned short)` and implement:

```cpp
bool DfjzhMotionController::setCurrentPositionAsOrigin(int axis, QString* error)
{
    if (!validAxis(axis, error) || homeAxis_(kBoardNo, static_cast<unsigned short>(axis)) != 0) {
        setMessage(error, "Motion card failed to zero axis " + QString::number(axis));
        return false;
    }
    positions_[static_cast<size_t>(axis)] = 0;
    return true;
}

bool DfjzhMotionController::moveTo(int axis, long target, const PrintAxisConfig& config, QString* error)
{
    // SetAxis* calls use target directly; do not apply changeDirection here.
    // The legacy runner determines the signed X/Y target before calling this method.
    // Then issue SetAxisAcc/Dec/Vel/StartVel/StopVel/Pos and StartAxis_ID.
}
```

Make `loadLibrary`, `resolveRequiredFunctions`, and nonzero `InitCard_ID` return
`false` with an error and leave `initialized_` false. Remove `isSimulation`,
`enterSimulation`, and all success paths guarded by `simulation_` from both the
interface and adapter. `armExposureWindow` must retain the legacy calls
`SetAxisIO_ID(0, 1, 6, 1, 1, 1)`, `Set_IO_Pos_ID(0, 1, begin, end)`, and
`Enable_IO_Pos_ID(0, 1, 1)` in that order. `disarmExposureWindow` must retain
`Enable_IO_Pos_ID(0, 1, 0)`, `SetAxisIO_ID(0, 1, 6, 0, 1, 1)`, and
`WriteIoBit_ID(0, 0, 1)`. `shutdown()` must always disarm before `ExitCard_ID`
when the card initialized successfully.

- [ ] **Step 4: Run the tests to verify they pass**

Run the Task 1 test command.

Expected: `printing module tests passed`, including the missing-DLL assertion.

- [ ] **Step 5: Commit the strict adapter**

```powershell
git add printing/IMotionController.h printing/DfjzhMotionController.h printing/DfjzhMotionController.cpp printing/tests/test_printing_modules.cpp printing/tests/printing_tests.pro
git commit -m "fix: fail 9030 jobs when motion card is unavailable"
```

### Task 3: Port the Legacy Second Screen and D3D11 Renderer

**Files:**
- Create: `printing/IPrintFramePresenter.h`
- Create: `printing/LegacyD3DImageRenderer.h`
- Create: `printing/LegacyD3DImageRenderer.cpp`
- Create: `printing/LegacySecondScreenPresenter.h`
- Create: `printing/LegacySecondScreenPresenter.cpp`
- Modify: `mergeholo.pro`

**Consumes:** `PrintFrame`, `QWidget::winId()`, Windows monitor APIs.

**Produces:** GUI-thread `LegacySecondScreenPresenter` implementing the presenter interface.

- [ ] **Step 1: Add the failing compile-time presenter contract to the tests**

Add a simple fake presenter used by later runner tests:

```cpp
class RecordingPresenter final : public IPrintFramePresenter {
public:
    bool prepare(const PrintFrame&, QSize, QString*) override { events << "prepare"; return prepareResult; }
    bool present(const PrintFrame&, QSize, QString*) override { events << "present"; return presentResult; }
    bool waitForVBlank(QString*) override { events << "vblank"; return vblankResult; }
    void shutdown() override { events << "shutdown"; }

    bool prepareResult = true;
    bool presentResult = true;
    bool vblankResult = true;
    QStringList events;
};
```

- [ ] **Step 2: Run the test target to verify it fails**

Run the Task 1 test command.

Expected: compilation fails because `IPrintFramePresenter` does not exist.

- [ ] **Step 3: Port the D3D11 path and legacy monitor choice**

Create `IPrintFramePresenter` with the exact interface in this plan. Port
`C:\wzp\Holographicface\9030\D3DImageRenderer.*` into
`LegacyD3DImageRenderer.*`, retaining:

```cpp
swapDesc.BufferCount = 2;
swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
swapChain_->Present(0, 0);
swapChain_->GetContainingOutput(&output);
output->WaitForVBlank();
```

Replace `CImage` upload with `PrintFrame`: expand `Bgr24` to temporary BGRA and
copy `Bgra32` directly before `UpdateSubresource`. Create a black top-level Qt
widget at the selected monitor rectangle and a centered child surface sized to
`int(frame.width * widthScale)` by `int(frame.height * heightScale)`, matching
the original `CSecondScr::displayImage` geometry. Use the original
`EnumDisplayMonitors` callback rule, but return an error when it finds no
non-primary monitor. Do not call `DwmFlush`.

Each public presenter method must dispatch to its GUI-thread implementation with
`QMetaObject::invokeMethod(..., Qt::BlockingQueuedConnection)` unless it already
runs on that thread. `prepare` creates the window and renders the warm-up frame;
the runner performs the first separate VBlank wait.

Add the four new source/header paths to `mergeholo.pro`. Keep the legacy
`#pragma comment(lib, ...)` directives for `d3d11.lib`, `dxgi.lib`, and
`d3dcompiler.lib` so the existing MSVC qmake build links the ported renderer.

- [ ] **Step 4: Build the application to verify the port compiles**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Config release
```

Expected: build succeeds and `00-bin\mergeholo.exe` is produced. Do not start a
print job as part of this step.

- [ ] **Step 5: Commit the D3D second-screen port**

```powershell
git add printing/IPrintFramePresenter.h printing/LegacyD3DImageRenderer.h printing/LegacyD3DImageRenderer.cpp printing/LegacySecondScreenPresenter.h printing/LegacySecondScreenPresenter.cpp mergeholo.pro
git commit -m "feat: port legacy 9030 second-screen renderer"
```

### Task 4: Isolate and Test the Legacy Timing Formula

**Files:**
- Create: `printing/LegacyPrintTiming.h`
- Create: `printing/LegacyPrintTiming.cpp`
- Modify: `printing/tests/test_printing_modules.cpp`
- Modify: `printing/tests/printing_tests.pro`
- Modify: `mergeholo.pro`

**Consumes:** `Print9030Config`.

**Produces:** deterministic `LegacyPrintTiming` and `LegacyRowPlan` values for the runner.

- [ ] **Step 1: Write failing forward/reverse timing tests**

Use the default Y settings with `gridRows = 2`, `gridColumns = 3`, and
`columnSpacingMm = 0.5`:

```cpp
void testLegacyTimingMatchesDefault9030Scan()
{
    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 2;
    config.main.gridColumns = 3;
    LegacyPrintTiming timing;
    QString error;
    expect(calculateLegacyPrintTiming(config, &timing, &error), "default timing should be valid");
    expect(timing.yStepPulse == 800, "Y step should be 800 pulses");
    expect(timing.accelerationPulse == 5850, "legacy acceleration pulse should match");
    expect(timing.framesPerImage == 1, "default scan must fit one 60 Hz frame per image");

    const LegacyRowPlan forward = makeLegacyRowPlan(timing, config, 0, false);
    expect(forward.compareBegin == 5850 && forward.compareEnd == LONG_MAX, "forward compare window should match legacy");
    const LegacyRowPlan reverse = makeLegacyRowPlan(timing, config, forward.yTarget, true);
    expect(reverse.compareBegin == LONG_MIN && reverse.compareEnd == 9050, "reverse compare window should match legacy");
}
```

Add a second test setting `axisY.speedOfMovement = 45000` and assert validation
fails with a VBlank frame-fit error.

- [ ] **Step 2: Run the tests to verify they fail**

Run the Task 1 test command.

Expected: compilation fails because the timing types and functions do not exist.

- [ ] **Step 3: Implement the pure timing calculator**

Use exactly these calculations and validations:

```cpp
const double accTime = double(axisY.speedOfMovement - axisY.startSpeed)
    / double(axisY.acceleratedVelocity);
timing.accelerationPulse = qRound64((axisY.startSpeed + axisY.speedOfMovement) * accTime * 0.5);
timing.yStepPulse = qRound64(axisY.subdivision * axisY.resolution * std::abs(config.main.columnSpacingMm));
timing.xStepPulse = qRound64(axisX.subdivision * axisX.resolution * std::abs(config.main.columnSpacingMm));
timing.exposurePulse = timing.yStepPulse * config.main.gridColumns;
timing.totalPulse = timing.exposurePulse + 2 * timing.accelerationPulse + config.main.addTempPulse;
```

Calculate `rawFramesPerImage = (timing.yStepPulse * 1'000'000.0 /
axisY.speedOfMovement) / (1'000'000.0 / 60.0)`, round to the nearest positive
integer, and reject an absolute fit error above `0.05`. `makeLegacyRowPlan`
must produce the exact forward/reverse boundaries from the approved design.

- [ ] **Step 4: Run the tests to verify they pass**

Run the Task 1 test command.

Expected: `printing module tests passed`.

- [ ] **Step 5: Commit the timing component**

```powershell
git add printing/LegacyPrintTiming.h printing/LegacyPrintTiming.cpp printing/tests/test_printing_modules.cpp printing/tests/printing_tests.pro mergeholo.pro
git commit -m "feat: calculate legacy 9030 scan timing"
```

### Task 5: Drive the Full Job Through Test Doubles

**Files:**
- Modify: `printing/PrintJobRunner.h`
- Modify: `printing/PrintJobRunner.cpp`
- Modify: `printing/tests/test_printing_modules.cpp`
- Modify: `printing/tests/printing_tests.pro`
- Modify: `mergeholo.pro`

**Consumes:** `IMotionController`, `IPrintFramePresenter`, `LegacyPrintTiming`, `PrintFrame`.

**Produces:** a runner that performs preflight, real-card scan calls, and unified cleanup.

- [ ] **Step 1: Write failing runner-order tests**

Add `RecordingMotionController` implementing `IMotionController`, recording
`initialize`, `origin:<axis>`, `move:<axis>:<target>`, `arm:<begin>:<end>`,
`o1:0`, `disarm`, `stop:<axis>`, and `shutdown`. Add an asynchronous helper:

```cpp
struct JobResult { bool success = false; QString text; };

JobResult runJobAndWait(PrintJobRunner& runner, const Print9030Config& config,
                        const PrintImageSet& images,
                        std::shared_ptr<IPrintFramePresenter> presenter)
{
    QEventLoop loop;
    JobResult result;
    QObject::connect(&runner, &PrintJobRunner::finished, &loop,
        [&result, &loop](bool success, const QString& text) {
            result = { success, text };
            loop.quit();
        });
    QString error;
    expect(runner.start(config, images, std::move(presenter), &error), "runner should start");
    loop.exec();
    return result;
}
```

Write these tests:

- `testSecondScreenFailureDoesNotInitializeCard`: `prepareResult = false`; assert
  failure, `initialize` absent, and no `move:` event.
- `testCardInitializationFailureFinishesWithoutMotion`: fake `initialize` returns
  false; assert failure, no `origin:` and no `move:` event.
- `testRunnerExecutesOneRowAndCleansUp`: use three memory frames and sampled Y
  positions at `yConstBegin`, `yConstBegin + yStepPulse`, and `yConstEnd`; assert
  presenter events are `prepare, vblank, present, vblank, present, vblank, present`;
  assert one `arm`, one Y move, `disarm`, O1-low, X/Y zero moves, and `shutdown`.
- `testRunnerReversesSecondRow`: use six frames; assert the second row presents
  indexes `5, 4, 3` and performs one X step before it.
- `testRunnerCancelsWithO1LowAndReturnToZero`: make the fake presenter call a
  supplied `runner.cancel()` callback after its first `present`; assert failure,
  `disarm`, O1-low, X/Y zero moves, and `shutdown`.

- [ ] **Step 2: Run the tests to verify they fail**

Run the Task 1 test command.

Expected: compilation fails because `start` does not accept an injected presenter
and the runner does not execute the recorded sequence.

- [ ] **Step 3: Implement the injected runner and cleanup guard**

Add this public API while keeping the existing dialog constructor valid:

```cpp
using MotionControllerFactory = std::function<std::unique_ptr<IMotionController>()>;

explicit PrintJobRunner(MotionControllerFactory controllerFactory = {}, QObject* parent = nullptr);
bool start(const Print9030Config& config, const PrintImageSet& images,
           std::shared_ptr<IPrintFramePresenter> presenter,
           QString* errorMessage = nullptr);
```

Use `DfjzhMotionController` when no factory is supplied. Before creating the
controller, decode frame zero, call `presenter->prepare`, and call
`presenter->waitForVBlank`; fail immediately on either error. Decode all frame
data before motion begins so an unreadable image cannot interrupt an armed scan.

After card initialization, call `setCurrentPositionAsOrigin` for axes 0 through
3 and force `setExposureOutput(false, 1)`. For every row, use
`LegacyRowPlan`, issue `moveTo(3, 0, axisW)` when W electrical status is true,
arm O1, execute `moveTo(1, row.yTarget, axisY)`, and select images from current
Y positions in forward order or reverse order. Present every selected frame and
wait exactly `framesPerImage` VBlank calls before considering the next image.
After Y stops, disarm and force O1 low before the X row move.

Implement one cleanup lambda that always runs after a successfully initialized
card. It disarms O1, forces output low, stops axes 0 through 3, moves X and Y to
target zero, waits for each, shuts down the card, then shuts down the presenter.
Append cleanup errors to the primary error; only emit successful `finished` after
cleanup succeeds. Remove the old read-only image-byte loop and simulation success
message.

- [ ] **Step 4: Run the tests to verify they pass**

Run the Task 1 test command.

Expected: `printing module tests passed`, with every fake-sequence assertion
passing and no motion initiated by display/card preflight failures.

- [ ] **Step 5: Commit the runner sequence**

```powershell
git add printing/PrintJobRunner.h printing/PrintJobRunner.cpp printing/tests/test_printing_modules.cpp printing/tests/printing_tests.pro mergeholo.pro
git commit -m "feat: execute legacy 9030 print sequence"
```

### Task 6: Wire the Presenter Into the Dialog and Build the App

**Files:**
- Modify: `widgets/Print9030Dialog.h`
- Modify: `widgets/Print9030Dialog.cpp`
- Modify: `mergeholo.pro`

**Consumes:** `LegacySecondScreenPresenter`, injected `PrintJobRunner::start`.

**Produces:** UI-started jobs with GUI-thread D3D presentation and existing failure popup behavior.

- [ ] **Step 1: Write the failing dialog integration assertion**

Add a focused code-level test helper to the console target that verifies the
runner start signature requires a non-null `std::shared_ptr<IPrintFramePresenter>`.
This test must reject a null presenter with a synchronous `false` return and the
message `"9030 second-screen presenter is unavailable."`.

- [ ] **Step 2: Run the test to verify it fails**

Run the Task 1 test command.

Expected: the runner currently accepts no presenter argument and cannot return
the required precondition error.

- [ ] **Step 3: Create and retain the GUI-thread presenter in the dialog**

In `Print9030Dialog::startPrint`, create the presenter before the worker starts:

```cpp
screenPresenter_ = std::make_shared<LegacySecondScreenPresenter>(this);
if (!runner_->start(config_, active, screenPresenter_, &error)) {
    screenPresenter_.reset();
    QMessageBox::warning(this, "9030 print", error);
    return;
}
```

Store `std::shared_ptr<IPrintFramePresenter> screenPresenter_` as a member.
Reset it only in the existing `finished` handler after the worker has emitted its
result. Keep the existing `QMessageBox::warning` on `finished(false, text)`;
this is the required initialization-failure popup path. Add the presenter sources
and headers to `mergeholo.pro` if Task 3 did not already do so.

- [ ] **Step 4: Run tests and the Release build**

Run:

```powershell
$buildDir = "$PWD\FF-tmp\printing-tests"
Push-Location $buildDir
cmd /d /c 'call "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && "C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe" "..\..\printing\tests\printing_tests.pro" "CONFIG+=release" -o Makefile && nmake /NOLOGO /f Makefile && release\printing_tests.exe'
Pop-Location
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Config release
```

Expected: `printing module tests passed` and a successful Release build with
`00-bin\mergeholo.exe` plus the deployed control-card DLLs.

- [ ] **Step 5: Commit the UI integration**

```powershell
git add widgets/Print9030Dialog.h widgets/Print9030Dialog.cpp mergeholo.pro printing/tests/test_printing_modules.cpp printing/tests/printing_tests.pro
git commit -m "feat: start 9030 jobs with legacy second screen"
```

### Task 7: Perform Hardware-Safe Manual Verification

**Files:**
- No source changes expected.

**Consumes:** Release executable, deployed `DfjzhControlerDll.dll`, connected control card, and external display.

**Produces:** verified on-device behavior or a recorded hardware-specific failure.

- [ ] **Step 1: Verify display preflight denial**

Disconnect or disable every external screen, open the 9030 dialog, provide valid
images, and start. Expected: warning identifies the missing legacy second screen;
the card is not initialized and no axis moves.

- [ ] **Step 2: Verify card initialization denial**

With the external screen enabled, temporarily make the card unavailable, then
start. Expected: the dialog warning includes the DLL/export/`InitCard_ID` error;
the presentation preflight may run, but no axis moves.

- [ ] **Step 3: Verify a one-cell motion run**

Restore the card and display. In the dialog set `gridRows = 1`, `gridColumns = 1`,
choose one valid image, and keep the legacy default speed values. Expected: first
frame is visible on the legacy-selected external screen, one VBlank preflight
occurs, O1 is low after the run, and X/Y end at logical zero.

- [ ] **Step 4: Verify a two-row serpentine run and cancellation**

Set `gridRows = 2`, `gridColumns = 3` and supply six distinguishable images.
Expected: row one displays 0/1/2, row two displays 5/4/3, and X advances once
between rows. Repeat and cancel during row one. Expected: O1 turns low, movement
stops, X/Y return to logical zero, and the dialog reports cancellation.

## Plan Self-Review

- Spec coverage: Task 1 covers raw/folder images; Task 2 maps the real control
  card and removes simulation; Task 3 ports selection, D3D11, and VBlank; Task 4
  preserves the timing formula; Task 5 exercises the whole runner and cleanup;
  Task 6 preserves the popup path; Task 7 verifies connected hardware.
- Placeholder scan: no deferred implementation markers are used; every task has
  exact files, interfaces, tests, commands, and commit scope.
- Type consistency: `PrintFrame` feeds `IPrintFramePresenter`; the presenter and
  `IMotionController` feed `PrintJobRunner`; the dialog retains the same shared
  presenter passed into `start`.

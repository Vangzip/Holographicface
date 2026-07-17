# Capture Counterclockwise Orientation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rotate camera RGB and depth frames 90 degrees counterclockwise before preview, freezing, saving, point-cloud generation, multiview, elemental conversion, and printing.

**Architecture:** A small camera utility owns the exact OpenCV rotation operation and returns an independent matrix. Both camera consumers, `CaptureWindow` and `runCaptureSession`, transform RGB and three-channel depth immediately after dequeueing an SDK frame, before any preview, movement detection, freeze, or file write. Existing downstream stages remain unchanged and consume the corrected pair.

**Tech Stack:** C++17, OpenCV 4.5 `cv::rotate`, Qt 5.15 Core/Widgets, qmake, MSVC 2019 v142 x64.

## Global Constraints

- Rotation direction is exactly `cv::ROTATE_90_COUNTERCLOCKWISE`.
- RGB, `img3d`, and display `depthMap` use the same row/column transform.
- The rotated matrix owns its storage and preserves source type/channel count.
- Main-window preview, frozen frame, `0.jpg`, `0.tiff`, mesh, multiview, elemental, and printing use the rotated data exactly once.
- Standalone `--capture` preview, movement detection, and saved 2D/3D files use the same rotation.
- Do not change camera SDK parsing, depth XYZ components, calibration values, downstream stage code, print timing, second-screen, motion, exposure, cancellation, or homing behavior.
- Preserve current uncommitted native-UI and 9030 changes; do not revert or clean them.

---

## File Structure

**Create:**

- `camera/CaptureOrientation.h`: declares the dependency-light rotation utility.
- `camera/CaptureOrientation.cpp`: applies OpenCV counterclockwise rotation.
- `camera/tests/test_capture_orientation.cpp`: exact mapping, type, ownership, pair-alignment, and integration assertions.
- `camera/tests/capture_orientation_tests.pro`: isolated Qt Core/OpenCV console test.

**Modify:**

- `widgets/CaptureWindow.cpp`: rotates all three frame representations before publishing `latest*` values.
- `camera/CaptureSession.cpp`: rotates RGB and `img3d` before preview, change detection, and saving.
- `mergeholo.pro`: compiles and exposes the shared orientation utility without disturbing existing entries.

---

### Task 1: Rotation Utility Contract

**Files:**

- Create: `camera/CaptureOrientation.h`
- Create: `camera/CaptureOrientation.cpp`
- Create: `camera/tests/test_capture_orientation.cpp`
- Create: `camera/tests/capture_orientation_tests.pro`

**Interfaces:**

- Produces: `cv::Mat rotateCaptureCounterClockwise90(const cv::Mat& source)`.
- Guarantees: empty input returns empty; nonempty output has `source.cols` rows and `source.rows` columns; type and channels are preserved; result storage is independent.

- [ ] **Step 1: Write the failing mapping tests**

Create a console test with `expect(...)`. For a 2x3 single-channel matrix:

```text
1 2 3       3 6
4 5 6  ->   2 5
             1 4
```

Assert exact values, 3x2 output dimensions, unchanged type, and an empty result for empty input.

Create paired 2x3 matrices:

```cpp
cv::Mat rgb(2, 3, CV_8UC3);
cv::Mat depth(2, 3, CV_32FC3);
```

Store the same unique pixel ID in RGB channel 0 and depth channel 0, rotate both, and assert the IDs remain equal at every destination coordinate. Modify one output pixel and assert the source pixel is unchanged to prove independent ownership.

Use `QTemporaryDir` to save a rotated non-square RGB matrix as JPEG and a rotated `CV_32FC3` matrix as uncompressed TIFF. Reload both and assert dimensions are swapped; assert exact channel-0 IDs for TIFF and red/blue block dominance for JPEG.

- [ ] **Step 2: Add the isolated qmake target and run red**

The `.pro` uses Qt Core, includes `../../Pri/opencv.pri`, and compiles the test plus `../CaptureOrientation.cpp`.

Run from `build/capture_orientation_tests`:

```powershell
cmd.exe /d /s /c 'call "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && "C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe" "..\..\camera\tests\capture_orientation_tests.pro" "CONFIG+=release" -o Makefile && nmake /f Makefile.Release'
```

Expected: compile failure because `CaptureOrientation.h/.cpp` do not exist.

- [ ] **Step 3: Implement the minimal utility**

```cpp
cv::Mat rotateCaptureCounterClockwise90(const cv::Mat& source)
{
    if (source.empty()) {
        return {};
    }
    cv::Mat rotated;
    cv::rotate(source, rotated, cv::ROTATE_90_COUNTERCLOCKWISE);
    return rotated;
}
```

Use `<opencv2/core.hpp>` in the header and `<opencv2/imgproc.hpp>` in the implementation.

- [ ] **Step 4: Run green**

Build and run `release/capture_orientation_tests.exe` with the OpenCV runtime directory on `PATH`.

Expected: exit `0` and `capture orientation tests passed`.

---

### Task 2: Main Window and Standalone Capture Integration

**Files:**

- Modify: `camera/tests/test_capture_orientation.cpp`
- Modify: `widgets/CaptureWindow.cpp`
- Modify: `camera/CaptureSession.cpp`
- Modify: `mergeholo.pro`

**Interfaces:**

- Consumes: `rotateCaptureCounterClockwise90(const cv::Mat&)` from Task 1.
- Produces: rotated `latestRgb_`, `latestDepthForPipeline_`, `latestDepthDisplay_`, `image2d`, and `image3d` values before any consumer observes them.

- [ ] **Step 1: Add failing integration assertions**

Read `widgets/CaptureWindow.cpp` and `camera/CaptureSession.cpp` relative to `QCoreApplication::applicationDirPath()`. Assert the source contains these calls:

```cpp
rotateCaptureCounterClockwise90(frame.img2d)
rotateCaptureCounterClockwise90(frame.img3d)
rotateCaptureCounterClockwise90(data.img2d)
rotateCaptureCounterClockwise90(data.img3d)
```

Also assert `CaptureWindow.cpp` rotates `frame.depthMap` when it is present.

- [ ] **Step 2: Run red**

Build and run the orientation test.

Expected: failure stating the main-window RGB integration is missing.

- [ ] **Step 3: Integrate the main-window frame boundary**

Include `CaptureOrientation.h`. In `pollCameraFrame()`, compute local matrices first:

```cpp
cv::Mat orientedRgb = rotateCaptureCounterClockwise90(frame.img2d);
cv::Mat orientedDepth = rotateCaptureCounterClockwise90(frame.img3d);
cv::Mat orientedDepthDisplay = rotateCaptureCounterClockwise90(
    frame.depthMap.empty() ? frame.img3d : frame.depthMap);

latestRgb_ = std::move(orientedRgb);
latestDepthForPipeline_ = std::move(orientedDepth);
latestDepthDisplay_ = std::move(orientedDepthDisplay);
```

Publishing only after all three operations succeed prevents a mixed-orientation pair. Keep `captureFrame()`, preview, and `preparePipelineInput()` unchanged so they consume these values without a second rotation.

- [ ] **Step 4: Integrate standalone capture**

Include `CaptureOrientation.h`. Replace the two clones after `GetHoloOutData` with:

```cpp
cv::Mat image2d = rotateCaptureCounterClockwise90(data.img2d);
cv::Mat image3d = rotateCaptureCounterClockwise90(data.img3d);
```

Keep preview, movement detection, type conversion, JPEG/TIFF write, and `previousImage2d` assignment unchanged.

- [ ] **Step 5: Register the utility**

Add `camera/CaptureOrientation.cpp` to `SOURCES` and `camera/CaptureOrientation.h` to `HEADERS` in `mergeholo.pro`. Do not remove or reorder printing/native-style entries.

- [ ] **Step 6: Run green**

Rebuild and run the orientation test.

Expected: all exact mapping, alignment, ownership, empty-input, and both-consumer integration assertions pass.

---

### Task 3: Regression, Build, and Direction Review

**Files:**

- Verify: all Task 1-2 paths plus existing UI/pipeline modules.
- Modify only if verification exposes a defect caused by this change.

**Interfaces:**

- Consumes: corrected capture values.
- Produces: fresh test/build evidence and a visual confirmation that both previews are upright and aligned.

- [ ] **Step 1: Run focused and regression tests**

Run freshly:

```powershell
.\build\capture_orientation_tests\release\capture_orientation_tests.exe
$env:QT_QPA_PLATFORM='offscreen'; .\build\save_settings_dialog_tests\release\save_settings_dialog_tests.exe
.\build\result_persistence_tests\release\result_persistence_tests.exe
.\FF-tmp\printing-tests\release\printing_tests.exe
.\00-bin\elemental_tests\test_elemental_current.exe
```

Expected: every process exits `0`; persistence may print its intentional invalid-path warning.

- [ ] **Step 2: Run the full Release build**

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Config release -SkipDeploy
```

Expected: qmake/uic/compile/link exit `0` and `00-bin/mergeholo.exe` exists.

- [ ] **Step 3: Verify saved orientation without hardware dependency**

Confirm the focused orientation test's JPEG/TIFF serialization cases pass: dimensions are swapped, TIFF IDs retain the exact counterclockwise mapping, and JPEG color blocks have the expected channel dominance.

- [ ] **Step 4: Visual hardware check when the camera is available**

Launch `00-bin/mergeholo.exe --ui` and confirm RGB/depth are both upright and spatially aligned. Capture one frame, process it, and confirm the generated print sequence is upright. If the physical camera or printer is unavailable, report that limitation rather than claiming hardware verification.

- [ ] **Step 5: Final diff and review**

Run `git diff --check` on touched paths. Confirm no downstream mesh/multiview/elemental/printing method was modified and no duplicate rotation exists after `pollCameraFrame()`/`runCaptureSession()`.

- [ ] **Step 6: Commit only isolated orientation paths**

Commit helper/tests and non-overlapping integration files only. Leave `mergeholo.pro` or `CaptureWindow.cpp` unstaged if separating the orientation hunks from existing user/UI work would risk including unrelated changes.

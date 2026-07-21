# Configurable Camera Orientation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the fixed inverted camera display upright by selecting a shared clockwise-90 or counterclockwise-90 capture orientation from `default_camera.ini` and the P2 device settings page.

**Architecture:** `CaptureOrientation` owns the orientation enum, text conversion, pixel rotation, and spatial XYZ rotation. `CameraCaptureSettings` stores the selected enum; `ProcessingSettingsStore` persists it; both `CaptureWindow` and `CaptureSession` call the same orientation-aware functions exactly once per frame.

**Tech Stack:** C++17, Qt 5.15 Widgets, OpenCV 4.5, qmake/MSVC2019, existing hand-written console tests.

## Global Constraints

- The new installation default is `capture_rotation=clockwise_90`.
- Supported values are exactly `clockwise_90` and `counterclockwise_90`.
- Clockwise spatial depth maps `(X, Y, Z)` to `(-Y, X, Z)`; counterclockwise maps it to `(Y, -X, Z)`.
- RGB, displayed depth, spatial depth, saved capture files, UI capture, and CLI capture must use the same orientation.
- Do not change multiview orbit, `multiview_capture_flip_vertical`, Elemental direction settings, or SDK calibration files.
- Stop a running `mergeholo.exe` before the official build; build only `00-bin/mergeholo.exe` and do not create `mergeholo_verify.exe`.
- Preserve unrelated dirty-worktree changes and stage only reviewed feature hunks.

---

### Task 1: Typed orientation and camera configuration

**Files:**
- Modify: `camera/CaptureOrientation.h`
- Modify: `camera/CaptureOrientation.cpp`
- Modify: `settings/ProcessingSettings.h`
- Modify: `settings/ProcessingSettingsStore.cpp`
- Modify: `config/default_camera.ini`
- Test: `widgets/tests/test_processing_settings.cpp`

**Interfaces:**
- Produces: `enum class CaptureRotation { Clockwise90, CounterClockwise90 };`
- Produces: `const char* captureRotationName(CaptureRotation)`.
- Produces: `bool parseCaptureRotation(const std::string&, CaptureRotation*)`.
- Adds: `CameraCaptureSettings::rotation`, defaulting to `CaptureRotation::Clockwise90`.

- [ ] **Step 1: Write failing configuration tests**

Extend the temporary `default_camera.ini` fixture and store tests:

```cpp
writeFile(QDir(configDirectory).filePath("default_camera.ini"),
    "camera_config_dir=084C\n"
    "capture_rotation=counterclockwise_90\n");

expect(loadProcessingSettings(paths, &settings, &error), qPrintable(error));
expect(settings.camera.rotation == CaptureRotation::CounterClockwise90,
    "camera rotation must load");

settings.camera.rotation = CaptureRotation::Clockwise90;
expect(saveProcessingSettings(paths, settings, &error), qPrintable(error));
expect(readFile(paths.cameraConfig).contains("capture_rotation=clockwise_90"),
    "camera rotation must persist");
```

Add a fixture without `capture_rotation` and assert `Clockwise90`. Add an invalid value and assert the same safe fallback.

- [ ] **Step 2: Run the focused test and verify it fails**

Run from `widgets/tests`:

```powershell
qmake processing_settings_tests.pro -spec win32-msvc CONFIG+=release
nmake /f Makefile.Release
release\processing_settings_tests.exe -o -,txt
```

Expected: compile failure because `CaptureRotation` and `CameraCaptureSettings::rotation` do not exist.

- [ ] **Step 3: Implement the enum and text conversion**

Add to `CaptureOrientation.h`:

```cpp
#include <string>

enum class CaptureRotation {
    Clockwise90,
    CounterClockwise90
};

const char* captureRotationName(CaptureRotation rotation);
bool parseCaptureRotation(const std::string& text, CaptureRotation* rotation);
```

Implement exact, case-insensitive parsing in `CaptureOrientation.cpp`; return `false` without changing the output for unknown values.

- [ ] **Step 4: Add the setting and persistence**

Add to `CameraCaptureSettings`:

```cpp
CaptureRotation rotation = CaptureRotation::Clockwise90;
```

In `loadProcessingSettings`, parse `capture_rotation` into a local copy and assign only on success. In `saveProcessingSettings`, write:

```cpp
camera.setValue("capture_rotation",
    QString::fromLatin1(captureRotationName(settings.camera.rotation)));
```

Append `capture_rotation=clockwise_90` to `config/default_camera.ini`.

- [ ] **Step 5: Run focused tests and commit the typed configuration**

Expected: `processing settings tests passed`.

Stage only these feature hunks and commit:

```powershell
git add -p camera/CaptureOrientation.h camera/CaptureOrientation.cpp settings/ProcessingSettings.h settings/ProcessingSettingsStore.cpp config/default_camera.ini widgets/tests/test_processing_settings.cpp
git commit -m "feat: configure camera capture orientation"
```

---

### Task 2: Orientation-aware RGB and spatial-depth transforms

**Files:**
- Modify: `camera/CaptureOrientation.h`
- Modify: `camera/CaptureOrientation.cpp`
- Modify: `camera/tests/test_capture_orientation.cpp`

**Interfaces:**
- Produces: `cv::Mat rotateCaptureImage(const cv::Mat&, CaptureRotation)`.
- Produces: `cv::Mat rotateCaptureSpatialDepth(const cv::Mat&, CaptureRotation)`.
- Removes consumer dependence on the fixed `rotateCaptureCounterClockwise90` and `rotateCaptureDepthCounterClockwise90` helpers.

- [ ] **Step 1: Replace fixed-direction tests with two-direction failing tests**

For a source matrix:

```cpp
cv::Mat source = (cv::Mat_<unsigned char>(2, 3) << 1, 2, 3, 4, 5, 6);
const cv::Mat clockwise = rotateCaptureImage(source, CaptureRotation::Clockwise90);
const unsigned char expectedClockwise[3][2] = {
    { 4, 1 }, { 5, 2 }, { 6, 3 }
};
const cv::Mat counterclockwise = rotateCaptureImage(
    source, CaptureRotation::CounterClockwise90);
const unsigned char expectedCounterclockwise[3][2] = {
    { 3, 6 }, { 2, 5 }, { 1, 4 }
};
```

For spatial depth, assert clockwise transforms `(X,Y,Z)` to `(-Y,X,Z)` and counterclockwise preserves `(Y,-X,Z)`. Retain RGB/depth pixel-ID alignment and non-aliasing assertions.

- [ ] **Step 2: Run orientation tests and verify compile failure**

Run from `camera/tests`:

```powershell
qmake capture_orientation_tests.pro -spec win32-msvc CONFIG+=release
nmake /f Makefile.Release
release\capture_orientation_tests.exe -o -,txt
```

Expected: compile failure for missing orientation-aware helpers.

- [ ] **Step 3: Implement the two transforms**

Implement pixel rotation with the matching OpenCV rotate code:

```cpp
const int code = rotation == CaptureRotation::Clockwise90
    ? cv::ROTATE_90_CLOCKWISE
    : cv::ROTATE_90_COUNTERCLOCKWISE;
cv::rotate(source, rotated, code);
```

After spatial pixel rotation, transform each point:

```cpp
if (rotation == CaptureRotation::Clockwise90) {
    point[0] = -y;
    point[1] = x;
}
else {
    point[0] = y;
    point[1] = -x;
}
```

Preserve `Z`, the input type assertion `CV_32FC3`, empty-input handling, and copied output storage.

- [ ] **Step 4: Run orientation tests and commit**

Expected: `capture orientation tests passed`.

```powershell
git add -p camera/CaptureOrientation.h camera/CaptureOrientation.cpp camera/tests/test_capture_orientation.cpp
git commit -m "fix: rotate capture for inverted camera"
```

---

### Task 3: Device UI and capture consumers

**Files:**
- Modify: `widgets/ProcessingSettingsDialog.cpp`
- Modify: `widgets/CaptureWindow.cpp`
- Modify: `camera/CaptureSession.cpp`
- Test: `widgets/tests/test_processing_settings.cpp`
- Test: `camera/tests/test_capture_orientation.cpp`

**Interfaces:**
- Adds device control object name: `cameraRotationCombo`.
- Consumes: `CameraCaptureSettings::rotation` in UI capture and CLI capture.

- [ ] **Step 1: Write failing binding and consumer-contract tests**

In the device-page test:

```cpp
settings.camera.rotation = CaptureRotation::CounterClockwise90;
dialog.setSettings(settings);
QComboBox* rotation = dialog.findChild<QComboBox*>("cameraRotationCombo");
expect(rotation != nullptr, "camera rotation control must exist");
expect(rotation->currentData().toInt()
        == static_cast<int>(CaptureRotation::CounterClockwise90),
    "camera rotation must populate");
rotation->setCurrentIndex(rotation->findData(
    static_cast<int>(CaptureRotation::Clockwise90)));
expect(dialog.settings().camera.rotation == CaptureRotation::Clockwise90,
    "camera rotation must collect");
```

Update source-contract assertions so both consumers contain:

```text
rotateCaptureImage(..., settings_.camera.rotation)
rotateCaptureSpatialDepth(..., settings_.camera.rotation)
rotateCaptureImage(..., options.cameraSettings.rotation)
rotateCaptureSpatialDepth(..., options.cameraSettings.rotation)
```

Assert each call occurs exactly once per consumer.

- [ ] **Step 2: Run both focused suites and verify failures**

Expected: missing `cameraRotationCombo` and old fixed helper source assertions.

- [ ] **Step 3: Add the P2 device control and binding**

Under `采集参数`, add:

```cpp
auto* rotation = new QComboBox(captureGroup);
rotation->setObjectName("cameraRotationCombo");
rotation->addItem(QString::fromUtf8("顺时针 90°（当前倒装）"),
    static_cast<int>(CaptureRotation::Clockwise90));
rotation->addItem(QString::fromUtf8("逆时针 90°（原安装）"),
    static_cast<int>(CaptureRotation::CounterClockwise90));
captureLayout->addRow(QString::fromUtf8("画面方向"), rotation);
```

Populate and collect it alongside exposure and frame rate. Add `rotation` to `sameCameraSettings` so Apply uses the existing safe camera restart path.

- [ ] **Step 4: Replace both consumers**

In `CaptureWindow::pollCameraFrame` use `settings_.camera.rotation` for RGB, spatial depth, and displayed depth. In `runCaptureSession`, use `options.cameraSettings.rotation` for saved/previewed RGB and spatial depth. Do not rotate any frame twice.

- [ ] **Step 5: Run both suites and commit**

Expected: `processing settings tests passed` and `capture orientation tests passed`.

```powershell
git add -p widgets/ProcessingSettingsDialog.cpp widgets/CaptureWindow.cpp camera/CaptureSession.cpp widgets/tests/test_processing_settings.cpp camera/tests/test_capture_orientation.cpp
git commit -m "feat: apply configured camera orientation"
```

---

### Task 4: Regression verification and official rebuild

**Files:**
- Modify only files needed to correct a verified failure.

**Interfaces:**
- Produces the official `00-bin/mergeholo.exe` only.

- [ ] **Step 1: Build and run all five regression suites**

Build and run:

```text
widgets/tests/release/processing_settings_tests.exe
widgets/tests/release/save_settings_dialog_tests.exe
camera/tests/release/capture_orientation_tests.exe
printing/tests/release/printing_tests.exe
pipeline/tests/release/result_persistence_tests.exe
```

Expected: all five exit `0` and print their pass messages.

- [ ] **Step 2: Stop the running official application**

Resolve processes by name and terminate only `mergeholo`:

```powershell
Get-Process -Name mergeholo -ErrorAction SilentlyContinue |
    Stop-Process -Force
```

Confirm the process count is zero before linking.

- [ ] **Step 3: Remove any obsolete verification executable**

After confirming the exact workspace path, delete only:

```text
C:\wzp\Holographicface\mergeholo\00-bin\mergeholo_verify.exe
```

Expected: `Test-Path` returns `False`.

- [ ] **Step 4: Rebuild the official release target**

Run qmake and `nmake /f Makefile.Release` without overriding `TARGET` or `DESTDIR_TARGET`.

Expected linker output:

```text
/OUT:00-bin\mergeholo.exe
```

- [ ] **Step 5: Smoke-test the official executable and review scope**

Run:

```powershell
.\00-bin\mergeholo.exe --mergeholo-help
git diff --check -- camera settings widgets config apps mergeholo.pro
```

Expected: help exits `0`, `mergeholo_verify.exe` is absent, and no feature-file whitespace errors are reported.

- [ ] **Step 6: Report the operator setting**

Document that the current installation uses:

```ini
# config/default_camera.ini
capture_rotation=clockwise_90
```

and that the same value is available under `设置 → 设备 → 采集参数 → 画面方向`.

# Unified Processing Settings Dialog Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the three main-window settings buttons with one native `处理设置` dialog organized as P0 常用, P1 成像, P2 高级, P2 设备, and P3 打印, while preserving the existing standalone 9030 print dialog.

**Architecture:** Add a typed `ProcessingSettings` aggregate and key-preserving configuration adapters under `settings/`. Add one `ProcessingSettingsDialog` whose first four navigation entries edit a draft and whose P3 entry emits a request to open the existing `Print9030Dialog`. `CaptureWindow` owns the applied settings, generates runtime pipeline values from them, and uses shared camera settings instead of hard-coded capture parameters.

**Tech Stack:** C++17, Qt 5.15 Widgets, qmake/MSVC2019, existing pipeline configuration format, existing hand-written console-style widget tests.

## Global Constraints

- Preserve the native Windows/Qt gray-white style; do not add custom stylesheets, frameless windows, web cards, gradients, or rounded controls.
- Keep `Print9030Dialog`, `print_9030.ini`, `PrintJobRunner`, and motion-controller behavior unchanged.
- Do not expose or rewrite `param.txt`, `.cen`, or individual `jp.xml` values.
- Keep explicit CLI configuration files and current configuration key meanings compatible.
- Missing new camera keys must fall back to exposure mode `1`, exposure value `15000`, frame rate `6.0`, camera interface `571`, camera type `Indigo`, camera ID `0`, GPU ID `0`, and missed-frame threshold `100`.
- Do not modify unrelated dirty-worktree files or include them in feature commits.

---

## File Structure

- Create `settings/ProcessingSettings.h`: typed settings aggregate and public conversion/validation functions.
- Create `settings/ProcessingSettings.cpp`: defaults, subject-size conversion, derived counts, validation, and camera-config construction.
- Create `settings/KeyValueConfig.h`: small key-preserving text configuration interface.
- Create `settings/KeyValueConfig.cpp`: load/update/save implementation using `QSaveFile`.
- Create `settings/ProcessingSettingsStore.h`: paths and load/save interface for application-owned settings.
- Create `settings/ProcessingSettingsStore.cpp`: adapters for pipeline INI, point-cloud CFG, mesh CFG, and camera INI.
- Create `widgets/ProcessingSettingsDialog.h`: dialog contract, navigation, draft access, and P3 print signal.
- Create `widgets/ProcessingSettingsDialog.cpp`: native Qt control setup, page binding, derived summaries, validation, and browse actions.
- Create `ui/ProcessingSettingsDialog.ui`: fixed navigation, four stacked settings pages, footer, and no embedded print controls.
- Create `widgets/tests/test_processing_settings.cpp`: model, persistence, and dialog contract tests.
- Create `widgets/tests/processing_settings_tests.pro`: focused test target.
- Modify `widgets/CaptureWindow.h/.cpp`: replace three settings entry points with unified settings ownership and dialog wiring.
- Modify `ui/CaptureWindow.ui`: replace input/print/save buttons with one `settingsButton` before capture.
- Modify `config/ui_pipeline_template.ini`: parameterize all applied P0/P1/P2 pipeline fields.
- Modify `config/default_camera.ini`: persist ordinary camera defaults with backward-compatible keys.
- Modify `apps/mergeholo_main.cpp` and `camera/CaptureSession.cpp`: use the shared camera settings for CLI capture.
- Modify `mergeholo.pro`: compile the new settings and dialog units.

---

### Task 1: Typed settings model and validation

**Files:**
- Create: `settings/ProcessingSettings.h`
- Create: `settings/ProcessingSettings.cpp`
- Create: `widgets/tests/test_processing_settings.cpp`
- Create: `widgets/tests/processing_settings_tests.pro`

**Interfaces:**
- Produces: `ProcessingSettings defaultProcessingSettings(const QString&, const QString&)`.
- Produces: `QString validateProcessingSettings(const ProcessingSettings&)`.
- Produces: `int viewCountPerAxis(const ProcessingSettings&)`, `qint64 elementalImageCount(const ProcessingSettings&)`.
- Produces: `double subjectSizeFromDistanceScale(double)` and `double distanceScaleFromSubjectSize(double)`.
- Produces: `LightFieldCapture::HoloInData makeCameraInput(const CameraCaptureSettings&)`.

- [ ] **Step 1: Write failing model tests**

Add tests that assert the exact current defaults, the `2.0 <-> 2.0` subject-size conversion, `90 * 3 = 270`, `150 * 150 = 22500`, and rejection of zero dimensions, non-existing external input, and incomplete camera preset directories.

```cpp
void testProcessingSettingsDefaults()
{
    const ProcessingSettings settings = defaultProcessingSettings(
        "C:/MergeHolo", "C:/MergeHolo/config/084C");
    expect(settings.pipeline.multiviewAngle == 90, "default angle");
    expect(settings.pipeline.multiviewPer == 3, "default samples per degree");
    expect(settings.camera.exposureValue == 15000, "default exposure");
    expect(settings.camera.cameraInterface == "571", "default interface");
    expect(viewCountPerAxis(settings) == 270, "derived view count");
    expect(elementalImageCount(settings) == 22500, "derived elemental count");
    expect(std::abs(subjectSizeFromDistanceScale(2.0) - 2.0) < 1e-9,
        "subject-size display conversion");
}
```

- [ ] **Step 2: Run the focused test to verify it fails**

Run from `widgets/tests`:

```powershell
qmake processing_settings_tests.pro -spec win32-msvc CONFIG+=release
nmake /f Makefile.Release
```

Expected: compilation fails because `ProcessingSettings.h` does not exist.

- [ ] **Step 3: Implement the typed model**

Define focused nested structs:

```cpp
struct CameraCaptureSettings {
    QString configDirectory;
    int exposureMode = 1;
    int exposureValue = 15000;
    double frameRate = 6.0;
    QString cameraInterface = "571";
    QString cameraType = "Indigo";
    int cameraId = 0;
    int gpuId = 0;
    int missedFrameThreshold = 100;
};

struct PipelineUiSettings {
    QString outputRoot;
    int multiviewAngle = 90;
    int multiviewPer = 3;
    int multiviewResolution = 150;
    int targetRows = 150;
    int targetCols = 150;
    double subjectSize = 2.0;
    double centerX = 0.0;
    double centerY = 0.0;
    double centerZ = 0.0;
    double rotateXDeg = 0.0;
    double rotateZDeg = 0.0;
    int jpgQuality = 100;
    bool captureFlipVertical = true;
    bool elementalFlipSourceY = false;
    bool elementalFlipViewRows = false;
    int atlasSize = 0;
    int writerThreads = 0;
};

struct PointCloudUiSettings {
    double focus = 105.0;
    double disp = 1.0;
    double step = 0.02;
    bool outlierFilterEnabled = false;
};

struct MeshUiSettings {
    int reconstruct = 2;
    int kSearch = 20;
    double searchRadius = 0.01;
    double leafSize = 0.001;
};

struct ProcessingSettings {
    PipelineInputSelection input;
    ResultSaveSettings saveResults;
    PipelineUiSettings pipeline;
    PointCloudUiSettings pointCloud;
    MeshUiSettings mesh;
    CameraCaptureSettings camera;
};
```

Validate the exact spec ranges and require `jp.xml`, `param.txt`, and at least one `.cen` file for camera directories.

- [ ] **Step 4: Run the test and verify it passes**

Run the same qmake/nmake command and then:

```powershell
release\processing_settings_tests.exe
```

Expected: `processing settings tests passed`.

- [ ] **Step 5: Commit only Task 1 files**

```powershell
git add mergeholo/settings/ProcessingSettings.h mergeholo/settings/ProcessingSettings.cpp mergeholo/widgets/tests/test_processing_settings.cpp mergeholo/widgets/tests/processing_settings_tests.pro
git commit -m "feat: define unified processing settings"
```

---

### Task 2: Key-preserving configuration persistence

**Files:**
- Create: `settings/KeyValueConfig.h`
- Create: `settings/KeyValueConfig.cpp`
- Create: `settings/ProcessingSettingsStore.h`
- Create: `settings/ProcessingSettingsStore.cpp`
- Modify: `widgets/tests/test_processing_settings.cpp`
- Modify: `widgets/tests/processing_settings_tests.pro`

**Interfaces:**
- Consumes: `ProcessingSettings` from Task 1.
- Produces: `ProcessingSettingsPaths::fromProjectRoot(const QString&)`.
- Produces: `bool loadProcessingSettings(const ProcessingSettingsPaths&, ProcessingSettings*, QString*)`.
- Produces: `bool saveProcessingSettings(const ProcessingSettingsPaths&, const ProcessingSettings&, QString*)`.

- [ ] **Step 1: Add failing round-trip and preservation tests**

Use `QTemporaryDir` fixtures containing comments and unknown keys. Assert known values load, new camera keys fall back when absent, save changes only owned keys, comments and unknown keys remain, and failure does not truncate the original.

```cpp
writeText(cameraIni,
    "# camera comment\n"
    "camera_config_dir=084C\n"
    "vendor_extension=keep-me\n");
ProcessingSettings loaded;
QString error;
expect(loadProcessingSettings(paths, &loaded, &error), qPrintable(error));
loaded.camera.exposureValue = 12000;
expect(saveProcessingSettings(paths, loaded, &error), qPrintable(error));
const QByteArray saved = readText(cameraIni);
expect(saved.contains("# camera comment"), "comment must survive");
expect(saved.contains("vendor_extension=keep-me"), "unknown key must survive");
expect(saved.contains("exposure_value=12000"), "owned key must update");
```

- [ ] **Step 2: Run tests and verify the new cases fail**

Expected: missing `ProcessingSettingsStore.h` or unresolved store functions.

- [ ] **Step 3: Implement line-preserving updates and atomic saves**

`KeyValueConfig` stores original lines and a case-insensitive key-to-line index. `setValue()` replaces only the value after `=`, appends missing keys, and leaves section headers, comments, blank lines, and unknown keys untouched. `save()` uses `QSaveFile::commit()`.

`ProcessingSettingsStore` reads/writes:

- pipeline fields from `default_pipeline.ini`;
- point-cloud fields from `depth_to_pointcloud_config.cfg`;
- mesh fields from `mesh_config.cfg`;
- camera fields from `default_camera.ini`.

Paths in the loaded settings are resolved relative to the containing configuration file; saved paths use relative paths when they remain under the project root and native absolute paths otherwise.

- [ ] **Step 4: Run focused tests and verify all persistence cases pass**

Expected: `processing settings tests passed` with no temporary-file residue.

- [ ] **Step 5: Commit only Task 2 files**

```powershell
git add mergeholo/settings/KeyValueConfig.h mergeholo/settings/KeyValueConfig.cpp mergeholo/settings/ProcessingSettingsStore.h mergeholo/settings/ProcessingSettingsStore.cpp mergeholo/widgets/tests/test_processing_settings.cpp mergeholo/widgets/tests/processing_settings_tests.pro
git commit -m "feat: persist unified processing settings"
```

---

### Task 3: Unified dialog shell, P0 page, and P3 print navigation

**Files:**
- Create: `widgets/ProcessingSettingsDialog.h`
- Create: `widgets/ProcessingSettingsDialog.cpp`
- Create: `ui/ProcessingSettingsDialog.ui`
- Modify: `widgets/tests/test_processing_settings.cpp`
- Modify: `widgets/tests/processing_settings_tests.pro`

**Interfaces:**
- Consumes: `ProcessingSettings` and `validateProcessingSettings()`.
- Produces: `void setSettings(const ProcessingSettings&)`.
- Produces: `ProcessingSettings settings() const`.
- Produces signal: `void printRequested()`.
- Produces: `void setBusy(bool)` and `void selectPage(int)` for state control and tests.

- [ ] **Step 1: Add failing dialog-contract tests**

Assert native chrome, `settingsNavigation` order and badges, four actual stacked pages, no print controls inside the device page, P3 emits `printRequested` without changing the draft, P0 derived summary updates, Cancel preserves the caller's model, and Apply returns an accepted valid draft.

```cpp
ProcessingSettingsDialog dialog;
QListWidget* navigation = dialog.findChild<QListWidget*>("settingsNavigation");
expect(navigation && navigation->count() == 5, "five priority entries");
expect(navigation->item(4)->text().contains("打印"), "P3 print entry");
expect(dialog.findChild<QWidget*>("devicePage")
        ->findChild<QGroupBox*>("printGroup") == nullptr,
    "device page must not embed printing");
```

- [ ] **Step 2: Run tests and verify they fail because the dialog is absent**

Expected: compile failure for `ProcessingSettingsDialog.h`.

- [ ] **Step 3: Implement the dialog shell and P0 controls**

Build the accepted 920x650 native layout in the `.ui` file:

- left `QListWidget` named `settingsNavigation`;
- `QStackedWidget` named `settingsPages`;
- pages named `commonPage`, `imagingPage`, `advancedPage`, and `devicePage`;
- footer buttons `restoreDefaultsButton`, `cancelButton`, and `applyButton`.

Populate P0 with input-source combo and directory browse, output browse, angle/per/resolution/target controls, derived summary, and three save checkboxes. Reuse `resolvePipelineInput()` before acceptance. Selecting P3 emits `printRequested()` and restores the previously selected settings page.

- [ ] **Step 4: Run focused tests and save a UI snapshot**

Run:

```powershell
$env:MERGEHOLO_SETTINGS_UI_SNAPSHOT='C:\wzp\Holographicface\mergeholo\output\processing-settings-common.png'
release\processing_settings_tests.exe
```

Expected: tests pass and the snapshot uses native controls with no stylesheet.

- [ ] **Step 5: Commit only Task 3 files**

```powershell
git add mergeholo/widgets/ProcessingSettingsDialog.h mergeholo/widgets/ProcessingSettingsDialog.cpp mergeholo/ui/ProcessingSettingsDialog.ui mergeholo/widgets/tests/test_processing_settings.cpp mergeholo/widgets/tests/processing_settings_tests.pro
git commit -m "feat: add unified settings dialog shell"
```

---

### Task 4: P1 imaging and P2 advanced pages

**Files:**
- Modify: `ui/ProcessingSettingsDialog.ui`
- Modify: `widgets/ProcessingSettingsDialog.cpp`
- Modify: `widgets/tests/test_processing_settings.cpp`

**Interfaces:**
- Consumes and updates `PipelineUiSettings`, `PointCloudUiSettings`, and `MeshUiSettings`.
- Keeps unexposed detailed CFG keys untouched through `ProcessingSettingsStore`.

- [ ] **Step 1: Add failing binding and reset tests**

Cover subject size, X/Y/Z offsets, Z/X rotations, JPEG quality, the four flip presets, point-cloud core fields, mesh algorithm/core fields, automatic atlas/thread behavior, and page-scoped default reset.

- [ ] **Step 2: Run tests and confirm missing controls fail**

Expected: `findChild()` assertions fail for `subjectSizeSpin`, `pointCloudFocusSpin`, and `reconstructCombo`.

- [ ] **Step 3: Implement P1 and P2 controls exactly as approved**

Use the approved group names and control ranges. `hardwareAdaptiveCheck` writes zero for atlas and writer threads and disables manual controls. `pointCloudDetailsButton` and `meshDetailsButton` open compact native subordinate dialogs for the complete detailed field sets; those dialogs update the same draft and have standard OK/Cancel buttons.

- [ ] **Step 4: Run focused tests and visually inspect P1/P2 snapshots**

Expected: all binding, conversion, reset, and native-style assertions pass.

- [ ] **Step 5: Commit only Task 4 files**

```powershell
git add mergeholo/ui/ProcessingSettingsDialog.ui mergeholo/widgets/ProcessingSettingsDialog.cpp mergeholo/widgets/tests/test_processing_settings.cpp
git commit -m "feat: add imaging and advanced settings pages"
```

---

### Task 5: P2 camera device page and shared camera construction

**Files:**
- Modify: `ui/ProcessingSettingsDialog.ui`
- Modify: `widgets/ProcessingSettingsDialog.h`
- Modify: `widgets/ProcessingSettingsDialog.cpp`
- Modify: `settings/ProcessingSettings.cpp`
- Modify: `widgets/tests/test_processing_settings.cpp`

**Interfaces:**
- Consumes: `CameraCaptureSettings`.
- Produces signal: `void cameraReinitializeRequested(const CameraCaptureSettings&)`.
- Produces exact `LightFieldCapture::HoloInData` through `makeCameraInput()`.

- [ ] **Step 1: Add failing device-page tests**

Assert configuration directory validation, exposure/frame binding, read-only interface/type/ID/GPU summary, engineer dialog edits, busy-state disabling, and exact conversion into `HoloInData`.

- [ ] **Step 2: Run tests and confirm device controls are absent**

Expected: assertions fail for `cameraConfigDirectoryEdit` and `cameraExposureValueSpin`.

- [ ] **Step 3: Implement device and engineer controls**

The main page exposes preset/directory, exposure mode `手动 (1)`, exposure value, frame rate, status, `测试连接`, and `重新初始化`. The engineer subdialog exposes interface, type, camera ID, GPU ID, and missed-frame threshold. It contains an explicit warning that it is a safety boundary, not authentication.

`makeCameraInput()` also fills the disabled serial-temperature values with the existing defaults so both UI and CLI camera construction are identical.

- [ ] **Step 4: Run focused tests and inspect the device-page snapshot**

Expected: all device binding, validation, and camera-input tests pass.

- [ ] **Step 5: Commit only Task 5 files**

```powershell
git add mergeholo/ui/ProcessingSettingsDialog.ui mergeholo/widgets/ProcessingSettingsDialog.h mergeholo/widgets/ProcessingSettingsDialog.cpp mergeholo/settings/ProcessingSettings.cpp mergeholo/widgets/tests/test_processing_settings.cpp
git commit -m "feat: add camera device settings"
```

---

### Task 6: Main-window integration and runtime configuration

**Files:**
- Modify: `widgets/CaptureWindow.h`
- Modify: `widgets/CaptureWindow.cpp`
- Modify: `ui/CaptureWindow.ui`
- Modify: `config/ui_pipeline_template.ini`
- Modify: `config/default_camera.ini`
- Modify: `mergeholo.pro`
- Modify: `widgets/tests/test_processing_settings.cpp`
- Modify: `widgets/tests/processing_settings_tests.pro`

**Interfaces:**
- Consumes: `ProcessingSettingsDialog`, `ProcessingSettingsStore`, and `makeCameraInput()`.
- Replaces: `openInputSettings()` and `openSaveSettings()` with `openProcessingSettings()`.
- Keeps: `openPrintSettings()` as the only 9030 dialog launcher.

- [ ] **Step 1: Add failing integration-contract tests**

Read `CaptureWindow.ui` and source text to assert one `settingsButton`, no old input/save/print settings buttons, dialog signal wiring for P3, all runtime-template placeholders, and no hard-coded camera values in `CaptureWindow::startCamera()`.

- [ ] **Step 2: Run tests and verify old button/source assertions fail**

Expected: failures mention `inputSettingsButton`, `saveSettingsButton`, and hard-coded exposure values.

- [ ] **Step 3: Wire the unified dialog and applied settings**

At construction, create `ProcessingSettingsPaths` from `projectRoot_`, load settings with current constructor camera path as fallback, and start the camera with `makeCameraInput(settings_.camera)`. `openProcessingSettings()` passes a draft, handles `printRequested` by closing/hiding the settings dialog before opening `Print9030Dialog`, validates/saves on Apply, applies input preview behavior, and reinitializes the camera only if camera settings changed and the state is idle.

The main button order becomes:

```text
拍照 | 确认 | 重新拍照 | 设置
```

`setState()` enables `settingsButton` whenever processing is not active. The dialog receives `setBusy(state == Processing || state == Starting)`.

- [ ] **Step 4: Parameterize runtime pipeline output**

Add placeholders for every P0/P1/P2 pipeline field and replace them in `writePipelineConfig()`, including distance scale converted from subject size, X/Y/Z offsets, X/Z rotations, JPEG quality, flip flags, atlas size, writer threads, and output root. Remove duplicated save/input dialog ownership while retaining `inputSettings_` and `saveSettings_` only if needed as aliases during the transition; the final code uses `settings_.input` and `settings_.saveResults` directly.

- [ ] **Step 5: Run focused tests and the existing widget tests**

Run both:

```powershell
release\processing_settings_tests.exe
..\release\save_settings_dialog_tests.exe
```

Expected: both print their pass messages.

- [ ] **Step 6: Commit only Task 6 files**

```powershell
git add mergeholo/widgets/CaptureWindow.h mergeholo/widgets/CaptureWindow.cpp mergeholo/ui/CaptureWindow.ui mergeholo/config/ui_pipeline_template.ini mergeholo/config/default_camera.ini mergeholo/mergeholo.pro mergeholo/widgets/tests/test_processing_settings.cpp mergeholo/widgets/tests/processing_settings_tests.pro
git commit -m "feat: integrate unified processing settings"
```

---

### Task 7: CLI camera settings compatibility

**Files:**
- Modify: `apps/mergeholo_main.cpp`
- Modify: `camera/CaptureSession.h`
- Modify: `camera/CaptureSession.cpp`
- Modify: `mergeholo.pro`
- Modify: `widgets/tests/test_processing_settings.cpp`

**Interfaces:**
- Consumes: `loadProcessingSettings()` and `makeCameraInput()`.
- Keeps `--camera-config` as an explicit highest-priority directory override.

- [ ] **Step 1: Add failing source-contract and precedence tests**

Assert CLI defaults come from `default_camera.ini`, `--camera-config` overrides only the directory, missing new keys preserve current defaults, and `CaptureSession.cpp` no longer duplicates the nine camera constants.

- [ ] **Step 2: Run tests and verify duplicated constants are detected**

Expected: source-contract test fails on `config.iHoloExposeVal = 15000` in `CaptureSession.cpp`.

- [ ] **Step 3: Pass typed camera settings through capture options**

Extend `CaptureOptions` with `CameraCaptureSettings camera`, populate it in `mergeholo_main.cpp`, apply the CLI directory override, and call `makeCameraInput(options.camera)` inside `runCaptureSession()`.

- [ ] **Step 4: Run focused tests and a CLI help/dry-run smoke test**

Run:

```powershell
release\processing_settings_tests.exe
..\..\00-bin\mergeholo.exe --mergeholo-help
```

Expected: tests pass and help still documents `--camera-config`.

- [ ] **Step 5: Commit only Task 7 files**

```powershell
git add mergeholo/apps/mergeholo_main.cpp mergeholo/camera/CaptureSession.h mergeholo/camera/CaptureSession.cpp mergeholo/mergeholo.pro mergeholo/widgets/tests/test_processing_settings.cpp
git commit -m "refactor: share camera settings across capture paths"
```

---

### Task 8: Full verification and UI review

**Files:**
- Modify only files required to correct failures found by this task.

**Interfaces:**
- Verifies the complete feature; produces no new public interface.

- [ ] **Step 1: Run all focused test executables**

Run the processing settings, existing widget, pipeline persistence, camera orientation, and printing test executables. Expected: every executable exits `0` and prints its pass message.

- [ ] **Step 2: Build the official release executable**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Config release
```

Expected: qmake/nmake and deployment finish successfully and `00-bin/mergeholo.exe` is updated.

- [ ] **Step 3: Perform configuration compatibility smoke checks**

Run pipeline dry-run with the current default and microtest configurations. Expected: configuration parses successfully, derived counts match, and no existing key becomes unknown or empty.

- [ ] **Step 4: Perform UI smoke review**

Open the application and verify P0, P1, P2, P2, P3 navigation; native styling; keyboard focus; Apply/Cancel; camera busy-state behavior; P3 opening the unchanged print dialog; and correct main-window button order.

- [ ] **Step 5: Review the final diff for scope and whitespace**

```powershell
git diff --check
git status --short
```

Expected: no whitespace errors and no unrelated user-owned files staged.

- [ ] **Step 6: Commit verification-only corrections, if any**

Stage only files changed to correct verified failures and commit with:

```powershell
git commit -m "test: verify unified processing settings"
```

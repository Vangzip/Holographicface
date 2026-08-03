# Pipeline Input Settings Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a native Qt input-settings workflow that starts the holographic pipeline from camera, RGB/depth, mesh, or multiview input while preserving the existing in-memory print result.

**Architecture:** A pipeline-owned input model validates file contracts and derives one stage plan for all callers. `CaptureWindow` owns only the confirmed selection and preview state; `HoloPipeline` resolves the same selection again, preloads mesh files into `MeshMemoryResult`, and executes the derived stages.

**Tech Stack:** C++17, Qt 5.15 Widgets/uic, OpenCV 4.5, PCL `PolygonMesh`, qmake, MSVC 2019 v142 x64.

## Global Constraints

- With no external input selected, preserve the current camera capture, counterclockwise 90-degree orientation, and all-memory pipeline.
- External RGB/depth images are already oriented and must not be rotated again.
- Input directories are mutually exclusive and read-only; do not copy, rename, clear, or delete their contents.
- Dialog Cancel clears the selection and restores the camera flow.
- Dialog Confirm stores the selection; only the main window Confirm starts processing.
- Result-save failures remain warnings and do not fail processing or printing.
- Use native Windows Qt Widgets styling with no custom thick black borders.
- Before the release build, terminate a running `mergeholo.exe`; publish only `00-bin/mergeholo.exe` and remove `mergeholo.next.exe`.

## File Structure

- `pipeline/PipelineInput.h/.cpp`: input modes, file contracts, validation, stage-plan derivation, and mesh preload.
- `pipeline/PipelineContext.h`: parsed external input fields in `HoloConfig`.
- `pipeline/PipelineConfig.cpp`: parse `input_mode` and `input_dir`.
- `pipeline/HoloPipeline.cpp`: consume the run plan and preload file-backed mesh input.
- `pipeline/PipelineLogger.cpp`: record selected input mode and directory.
- `pipeline/PipelineModule.pri`: compile the new pipeline module.
- `pipeline/tests/test_result_persistence.cpp`: focused input contract, run-plan, and mesh-loader tests.
- `pipeline/tests/result_persistence_tests.pro`: compile `PipelineInput.cpp` in the existing PCL/OpenCV test target.
- `widgets/InputSettingsDialog.h/.cpp` and `ui/InputSettingsDialog.ui`: native mutually exclusive folder chooser.
- `widgets/CaptureWindow.h/.cpp` and `ui/CaptureWindow.ui`: main button, external preview, state transitions, and config generation.
- `widgets/tests/test_save_settings_dialog.cpp` and `widgets/tests/save_settings_dialog_tests.pro`: dialog behavior and style regression coverage.
- `config/ui_pipeline_template.ini`: generated input-mode placeholders.
- `mergeholo.pro`: include the new dialog and pipeline sources/forms.

---

### Task 1: Pipeline Input Model, Validation, And Run Plan

**Files:**
- Create: `pipeline/PipelineInput.h`
- Create: `pipeline/PipelineInput.cpp`
- Modify: `pipeline/PipelineContext.h`
- Modify: `pipeline/PipelineConfig.cpp`
- Modify: `pipeline/PipelineModule.pri`
- Modify: `pipeline/tests/test_result_persistence.cpp`
- Modify: `pipeline/tests/result_persistence_tests.pro`

**Interfaces:**
- Produces: `enum class PipelineInputMode { Camera, RgbDepth, Mesh, Multiview }`.
- Produces: `PipelineInputSelection { PipelineInputMode mode; std::filesystem::path directory; }`.
- Produces: `PipelineInputFiles { rgbPath; depthPath; meshPath; multiviewDirectory; }`.
- Produces: `PipelineRunPlan makePipelineRunPlan(PipelineInputMode mode)`.
- Produces: `bool resolvePipelineInput(const PipelineInputSelection&, const MultiviewInputSpec&, PipelineInputFiles*, std::string*)`.
- Produces: `pipelineInputModeName()` and `parsePipelineInputMode()` for INI round trips.

- [x] **Step 1: Write failing run-plan and mode parsing tests**

Add tests that assert these exact plans:

```cpp
expectPlan(PipelineInputMode::Camera, true, true, true, true, true);
expectPlan(PipelineInputMode::RgbDepth, true, true, true, true, true);
expectPlan(PipelineInputMode::Mesh, false, false, false, true, true);
expectPlan(PipelineInputMode::Multiview, false, false, false, false, true);
expect(parsePipelineInputMode("rgb_depth") == PipelineInputMode::RgbDepth,
    "rgb_depth mode must parse");
expect(pipelineInputModeName(PipelineInputMode::Multiview) == "multiview",
    "multiview mode must serialize");
```

- [x] **Step 2: Run the pipeline test target and verify red**

Run from the existing MSVC developer environment:

```powershell
cmd.exe /d /s /c 'call "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && cd /d C:\wzp\Holographicface\mergeholo\pipeline\tests && "C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe" result_persistence_tests.pro "CONFIG+=release" -o Makefile && nmake /f Makefile.Release'
```

Expected: compile failure because `PipelineInput.h` and its functions do not exist.

- [x] **Step 3: Implement the input types and exact stage matrix**

Define focused value types:

```cpp
enum class PipelineInputMode { Camera, RgbDepth, Mesh, Multiview };

struct PipelineInputSelection {
    PipelineInputMode mode = PipelineInputMode::Camera;
    std::filesystem::path directory;
    bool isExternal() const { return mode != PipelineInputMode::Camera; }
    void clear() { mode = PipelineInputMode::Camera; directory.clear(); }
};

struct PipelineRunPlan {
    bool depth;
    bool mesh;
    bool model;
    bool multiview;
    bool elemental;
    bool preloadMesh;
    bool useFileMultiview;
};
```

Implement the four plans exactly as specified in the table above. Add `inputMode` and `inputDirectory` to `HoloConfig`, and parse `input_mode`/`input_dir` in `applyConfig()`.

- [x] **Step 4: Write failing directory contract tests**

Use temporary directories to cover:

```cpp
// RGB/depth accepts one matching pair only.
writeImage(root / "face.jpg", 4, 3);
writeDepth(root / "face.tiff", 4, 3);
expect(resolvePipelineInput(rgbSelection(root), smallSpec(), &files, &error),
    "matching RGB/depth pair must resolve");
expect(files.rgbPath.filename() == "face.jpg", "resolved RGB path is wrong");

// Mesh requires a colored <base>_mesh.ply plus a decodable <base>.jpg.
writeTriangleMesh(root / "face_mesh.ply");
writeImage(root / "face.jpg", 4, 3);
expect(resolvePipelineInput(meshSelection(root), smallSpec(), &files, &error),
    "matching mesh/texture pair must resolve");

// A 2x2 multiview spec requires all four zero-padded files.
writeImage(root / "0101.jpg", 4, 3);
writeImage(root / "0102.jpg", 4, 3);
writeImage(root / "0201.jpg", 4, 3);
writeImage(root / "0202.jpg", 4, 3);
expect(resolvePipelineInput(multiviewSelection(root), smallSpec(), &files, &error),
    "complete multiview grid must resolve");
```

Also assert failures for a missing directory, two TIFF pairs, missing mesh texture, missing middle multiview image, and a first multiview image with the wrong dimensions.

- [x] **Step 5: Implement deterministic validation**

Use `std::filesystem::directory_iterator` and require the exact lowercase suffixes consumed by the existing stages. RGB/depth must resolve one `.tiff` and its same-basename `.jpg`; mesh must resolve one `_mesh.ply` and its same-basename `.jpg`; multiview must check every expected filename and decode the first image to verify `viewWidth`/`viewHeight`. Return a concise Chinese-ready UTF-8 error string rather than printing or showing UI.

- [x] **Step 6: Run tests and verify green**

Run:

```powershell
pipeline\tests\release\result_persistence_tests.exe
```

Expected: exit `0` and `result persistence tests passed`.

- [ ] **Step 7: Commit the pipeline input model**

```powershell
git add mergeholo/pipeline/PipelineInput.h mergeholo/pipeline/PipelineInput.cpp mergeholo/pipeline/PipelineContext.h mergeholo/pipeline/PipelineConfig.cpp mergeholo/pipeline/PipelineModule.pri mergeholo/pipeline/tests/test_result_persistence.cpp mergeholo/pipeline/tests/result_persistence_tests.pro
git commit -m "feat: model external pipeline inputs"
```

---

### Task 2: Mesh Preload And Pipeline Stage Integration

**Files:**
- Modify: `pipeline/PipelineInput.h`
- Modify: `pipeline/PipelineInput.cpp`
- Modify: `pipeline/HoloPipeline.cpp`
- Modify: `pipeline/PipelineLogger.cpp`
- Modify: `pipeline/tests/test_result_persistence.cpp`

**Interfaces:**
- Consumes: `PipelineInputSelection`, `PipelineInputFiles`, and `PipelineRunPlan` from Task 1.
- Produces: `bool loadPipelineMeshInput(const PipelineInputFiles&, MeshMemoryResult*, std::string*)`.

- [x] **Step 1: Write a failing mesh preload test**

Create a triangle `pcl::PolygonMesh`, save it as `face_mesh.ply`, write `face.jpg`, resolve the selection, and assert:

```cpp
MeshMemoryResult loaded;
expect(loadPipelineMeshInput(files, &loaded, &error), "mesh input must load");
expect(loaded.hasMesh(), "loaded mesh must contain cloud and polygons");
expect(loaded.baseName == "face", "mesh basename must remove _mesh suffix");
expect(loaded.meshPath == files.meshPath, "logical mesh path must be preserved");
expect(loaded.rgbPath == files.rgbPath, "texture path must be preserved");
```

Add a corrupt PLY case that returns false and leaves the result empty.

- [x] **Step 2: Run the test and verify red**

Run `pipeline\tests\release\result_persistence_tests.exe`.

Expected: link or assertion failure because `loadPipelineMeshInput` is not implemented.

- [x] **Step 3: Implement mesh preload**

Allocate `pcl::PolygonMesh::Ptr`, load with `pcl::io::loadPLYFile`, require non-empty cloud data, polygons, and an RGB/RGBA vertex field, verify the companion JPEG is decodable, set paths/base name only after validation, and clear the output on every failure.

- [x] **Step 4: Integrate one run plan into `HoloPipeline`**

At pipeline startup:

```cpp
const PipelineInputSelection selection{config.inputMode, config.inputDirectory};
PipelineInputFiles inputFiles;
const PipelineRunPlan plan = makePipelineRunPlan(selection.mode);
if (!resolvePipelineInput(selection, specFrom(config), &inputFiles, &inputError)) {
    std::cerr << "[input] " << inputError << std::endl;
    return finish(1);
}
if (plan.preloadMesh && !loadPipelineMeshInput(inputFiles, &meshMemory, &inputError)) {
    std::cerr << "[input] " << inputError << std::endl;
    return finish(1);
}
// Keep multiviewOutDir as the generated-output destination. The elemental
// file path reads config.inputDirectory when useFileMultiview is active.
```

Combine `plan.depth/mesh/model/multiview/elemental` with the existing config flags and CLI stage filter. Treat a preloaded mesh as the same memory mesh used by the current renderer. Preserve the existing camera/RGB-depth memory decisions and result persistence behavior.

- [x] **Step 5: Add input details to the pipeline log**

Write:

```text
input.mode=camera|rgb_depth|mesh|multiview
input.directory=<normalized path or empty>
```

Do not log input files as generated output directories.

- [x] **Step 6: Run pipeline regression tests**

Run:

```powershell
pipeline\tests\release\result_persistence_tests.exe
```

Expected: all input and persistence tests pass, including all 8 result-save combinations and elemental orientation assertions.

- [ ] **Step 7: Commit pipeline integration**

```powershell
git add mergeholo/pipeline/PipelineInput.h mergeholo/pipeline/PipelineInput.cpp mergeholo/pipeline/HoloPipeline.cpp mergeholo/pipeline/PipelineLogger.cpp mergeholo/pipeline/tests/test_result_persistence.cpp
git commit -m "feat: resume pipeline from mesh or multiview"
```

---

### Task 3: Native Input Settings Dialog

**Files:**
- Create: `widgets/InputSettingsDialog.h`
- Create: `widgets/InputSettingsDialog.cpp`
- Create: `ui/InputSettingsDialog.ui`
- Modify: `widgets/tests/test_save_settings_dialog.cpp`
- Modify: `widgets/tests/save_settings_dialog_tests.pro`
- Modify: `mergeholo.pro`

**Interfaces:**
- Consumes: `PipelineInputSelection` and `resolvePipelineInput()` from Task 1.
- Produces: `setInputSettings(const PipelineInputSelection&)`, `inputSettings() const`, and `setDirectory(PipelineInputMode, const QString&)`.

- [x] **Step 1: Write failing dialog tests**

Add assertions that the dialog:

```cpp
InputSettingsDialog dialog;
dialog.setDirectory(PipelineInputMode::RgbDepth, rgbDir);
expect(!dialog.findChild<QLineEdit*>("rgbDepthPathEdit")->text().isEmpty(),
    "RGB/depth path must be shown");
dialog.setDirectory(PipelineInputMode::Mesh, meshDir);
expect(dialog.findChild<QLineEdit*>("rgbDepthPathEdit")->text().isEmpty(),
    "mesh selection must clear RGB/depth");
expect(dialog.inputSettings().mode == PipelineInputMode::Mesh,
    "mesh row must become active");
```

Also test Confirm preserves the active row; Cancel, Escape, direct reject, and system close clear all rows; all path fields are read-only; the form has native window chrome, a standard `QDialogButtonBox`, no custom close tool button, and no style sheet.

- [x] **Step 2: Run widget tests and verify red**

Build and run:

```powershell
cmd.exe /d /s /c 'call "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && cd /d C:\wzp\Holographicface\mergeholo\widgets\tests && "C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe" save_settings_dialog_tests.pro "CONFIG+=release" -o Makefile && nmake /f Makefile.Release'
$env:QT_QPA_PLATFORM='offscreen'; widgets\tests\release\save_settings_dialog_tests.exe
```

Expected: compile failure because `InputSettingsDialog` does not exist.

- [x] **Step 3: Build the native dialog form**

Use a `QFormLayout` or two-column grid with labels `RGB、深度图选择`, `mesh 选择`, and `multiview 选择`. Each row contains a read-only `QLineEdit` and a fixed-width `QPushButton` labelled `浏览`. Add a standard horizontal `QDialogButtonBox` with Ok/Cancel. Use native margins and spacing; do not add a form style sheet.

- [x] **Step 4: Implement selection, browsing, validation, and clear-on-cancel**

Each Browse handler calls `QFileDialog::getExistingDirectory`. A non-empty result calls `setDirectory()`, which normalizes the path, sets exactly one row, and clears the other two. Override `accept()` to call `resolvePipelineInput()` with the UI template's 270x270/150x150 specification and show `QMessageBox::warning` on validation failure. Override `reject()` and `closeEvent()` to clear all rows before rejecting.

- [x] **Step 5: Run widget tests and verify green**

Run with `QT_QPA_PLATFORM=offscreen`.

Expected: exit `0` and `save settings dialog tests passed`.

- [ ] **Step 6: Commit the dialog**

```powershell
git add mergeholo/widgets/InputSettingsDialog.h mergeholo/widgets/InputSettingsDialog.cpp mergeholo/ui/InputSettingsDialog.ui mergeholo/widgets/tests/test_save_settings_dialog.cpp mergeholo/widgets/tests/save_settings_dialog_tests.pro mergeholo/mergeholo.pro
git commit -m "feat: add native input settings dialog"
```

---

### Task 4: Capture Window External Input Workflow

**Files:**
- Modify: `widgets/CaptureWindow.h`
- Modify: `widgets/CaptureWindow.cpp`
- Modify: `ui/CaptureWindow.ui`
- Modify: `config/ui_pipeline_template.ini`
- Modify: `widgets/tests/test_save_settings_dialog.cpp`

**Interfaces:**
- Consumes: `InputSettingsDialog` and `PipelineInputSelection`.
- Produces: `openInputSettings()`, `applyInputSettings()`, `clearInputSettingsAndResumeCamera()`, and mode-aware config generation.

- [x] **Step 1: Write failing main-window form/config tests**

Extend the source-level UI regression test to assert:

```cpp
const int inputButton = uiText.indexOf("name=\"inputSettingsButton\"");
const int captureButton = uiText.indexOf("name=\"captureButton\"");
expect(inputButton >= 0 && inputButton < captureButton,
    "input settings must appear before capture");
expect(templateText.contains("input_mode={{input_mode}}"),
    "UI config must include input mode");
expect(templateText.contains("input_dir={{input_dir}}"),
    "UI config must include input directory");
```

Add a pure helper test, if needed, that maps Camera/RGB-depth/Mesh/Multiview selections to the expected generated `depth_input_dir`, `input_mode`, and `input_dir` values.

- [x] **Step 2: Run widget tests and verify red**

Run the offscreen widget test executable.

Expected: assertion failure because the new button and template placeholders are absent.

- [x] **Step 3: Add and wire the main-window button**

Insert `inputSettingsButton` before `captureButton`, with the same min/max dimensions and size policy as adjacent buttons. Store the pointer in `CaptureWindow`, connect it to `openInputSettings()`, and enable it whenever the window is not processing or starting. Keep it usable in camera Error state.

- [x] **Step 4: Apply confirmed external selections**

For RGB/depth, decode the resolved `.jpg` with `cv::IMREAD_COLOR` and `.tiff` with `cv::IMREAD_UNCHANGED`, assign the frozen mats without calling either rotation helper, and show the existing RGB/depth previews. For mesh or multiview, clear stale camera frames/previews, release the camera, set state Frozen, display a concise ready status, and enable main Confirm. While any external source is active, disable Capture and Retake.

On dialog reject, clear `PipelineInputSelection`, clear external preview state, and call `startCamera()` so the default workflow resumes or retries initialization.

- [x] **Step 5: Make processing and config generation mode-aware**

Allow `startProcessing()` when an external mesh or multiview source is valid even though frozen camera mats are empty. Call `preparePipelineInput()` only for Camera mode. For RGB/depth, set `depth_input_dir` to the selected directory without copying files. Write:

```ini
input_mode={{input_mode}}
input_dir={{input_dir}}
depth_input_dir={{depth_input_dir}}
```

Replace all placeholders with forward-slash absolute paths. Re-resolve RGB/depth and mesh immediately before launching the worker thread. For the 72,900-file multiview grid, trust the dialog validation on the UI thread and let the worker perform the required second validation so the main window does not repeat the full scan twice.

- [x] **Step 6: Run focused regressions**

Run:

```powershell
$env:QT_QPA_PLATFORM='offscreen'; widgets\tests\release\save_settings_dialog_tests.exe
pipeline\tests\release\result_persistence_tests.exe
camera\tests\release\capture_orientation_tests.exe
```

Expected: all three executables exit `0`; the orientation test confirms only camera frames receive the 90-degree transform.

- [ ] **Step 7: Commit the capture-window integration**

```powershell
git add mergeholo/widgets/CaptureWindow.h mergeholo/widgets/CaptureWindow.cpp mergeholo/ui/CaptureWindow.ui mergeholo/config/ui_pipeline_template.ini mergeholo/widgets/tests/test_save_settings_dialog.cpp
git commit -m "feat: start processing from selected input stage"
```

---

### Task 5: Release Build, Runtime Smoke, And Visual Review

**Files:**
- Verify: `00-bin/mergeholo.exe`
- Verify: `output/holo_config.ui.ini`
- Verify: `output/pipeline.log`

**Interfaces:**
- Consumes: all earlier tasks.
- Produces: the official release executable and verification evidence.

- [x] **Step 1: Inspect the complete diff**

Run:

```powershell
git diff --check
git diff -- mergeholo/pipeline mergeholo/widgets mergeholo/ui mergeholo/config/ui_pipeline_template.ini mergeholo/mergeholo.pro
```

Expected: no whitespace errors; no unrelated files changed by this feature.

- [x] **Step 2: Run every focused test again**

Run the pipeline, widget, camera-orientation, printing timing, second-screen, and print-runner test executables already present in the repository.

Expected: every executable exits `0`.

- [x] **Step 3: Terminate the official app and remove stale next binary**

Resolve exact absolute paths first. Stop only processes whose executable path is `C:\wzp\Holographicface\mergeholo\00-bin\mergeholo.exe` or `mergeholo.next.exe`. Delete only the verified `C:\wzp\Holographicface\mergeholo\00-bin\mergeholo.next.exe` path if present.

- [x] **Step 4: Clean-build the official release executable**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\scripts\build.ps1' -Config release -Clean
```

Expected: qmake/uic/compile/link exit `0` and `00-bin/mergeholo.exe` has a new timestamp.

- [x] **Step 5: Run CLI and UI smoke checks**

Run `00-bin\mergeholo.exe --help` and expect exit `0`. Launch the application, open Input Settings, and verify with computer-use screenshots at the normal desktop viewport:

- Input Settings is before Capture and matches adjacent native buttons.
- Dialog has native title bar, three aligned folder rows, standard buttons, and no thick black border.
- Selecting one row clears the other two.
- Cancel clears input mode and returns to camera preview.
- Confirm returns to the main window without starting processing.
- RGB/depth input shows files without another 90-degree rotation.

- [x] **Step 6: Verify generated plans without running the expensive full dataset**

Use small valid fixture directories plus `--dry-run` configs to verify log/console stage selection for RGB-depth, mesh, and multiview. Confirm the default generated config remains `input_mode=camera` and that no external input directory gains new files.

- [x] **Step 7: Record executable identity**

Report the official executable's last-write time and SHA-256. Confirm `mergeholo.next.exe` is absent and no `mergeholo*` process remains running.

---

## Self-Review

- Spec coverage: all confirmed UI, input-contract, stage-skip, orientation, error, persistence, and release requirements map to Tasks 1-5.
- Placeholder scan: no `TBD`, deferred implementation, or unspecified test step remains.
- Type consistency: the same `PipelineInputSelection`, `PipelineInputFiles`, `MultiviewInputSpec`, and `PipelineRunPlan` interfaces flow from validation through the dialog, capture window, and pipeline.
- Scope: imported source files are validated and consumed but never duplicated as saved outputs; existing result persistence remains unchanged.

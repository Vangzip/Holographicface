# Processing Result Save Settings Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a session-only Qt save-settings dialog and best-effort timestamped persistence for mesh, multiview, and elemental pipeline results without changing the in-memory print path.

**Architecture:** `CaptureWindow` owns a lightweight `ResultSaveSettings` value and freezes it into the generated UI pipeline INI with one millisecond timestamp per run. `HoloPipeline` remains memory-first and invokes a focused `ResultPersistence` module immediately after each in-memory stage; failures append `ResultSaveWarning` entries and never change the pipeline exit code or the `ElementalMemoryResult` sent to printing. A dedicated Qt dialog edits the session value, while pipeline and dialog console tests exercise configuration defaults, file compatibility, warning behavior, and cancel semantics.

**Tech Stack:** Qt 5.15 Widgets / C++17 / qmake, OpenCV 4.5 JPEG encoding, PCL 1.12 PLY IO, existing OSG multiview memory buffers, MSVC 2019 v142 x64.

## Global Constraints

- The three save switches default to `false` and are never persisted across application restarts.
- Mesh, multiview, and elemental switches are independent and support all eight combinations.
- One run uses one local timestamp formatted exactly as `yyyyMMdd_HHmmss_zzz`.
- Selected directories are named exactly `mesh_<timestamp>`, `multiview_<timestamp>`, and `elemental_<timestamp>` under `output`; historical directories are never cleaned or overwritten.
- Mesh output is `0_mesh.ply`; default multiview names are `001001.jpg...270270.jpg`; default elemental names are `001001.jpg...150150.jpg`.
- Multiview and elemental JPEG color order, vertical orientation, numbering, dimensions, and `jpg_quality` must match the legacy file path.
- Optional result persistence is best effort: directory creation, encoding, or file-write failures append warnings, keep partial files, and never alter processing success or block `PrintJobRunner`.
- Required capture input, runtime INI, and pipeline log preparation retain their existing fatal error behavior.
- The current worktree contains unrelated user changes; do not revert, reformat, stage, or commit them.

---

## File Structure

**Create:**

- `pipeline/ResultSaveSettings.h`: dependency-light session settings, warning, and report value types shared by pipeline and Qt UI.
- `pipeline/ResultPersistence.h`: public timestamped path and three best-effort writer declarations.
- `pipeline/ResultPersistence.cpp`: PLY/JPEG encoding, legacy-compatible naming, orientation, color conversion, and warning capture.
- `pipeline/tests/test_result_persistence.cpp`: config, path, writer, and warning regression tests using tiny synthetic data.
- `pipeline/tests/result_persistence_tests.pro`: isolated qmake console test target for pipeline persistence.
- `ui/SaveSettingsDialog.ui`: reference-layout dialog with three independent checkboxes and confirm/cancel/close controls.
- `widgets/SaveSettingsDialog.h`: dialog API for loading and reading `ResultSaveSettings`.
- `widgets/SaveSettingsDialog.cpp`: confirm, reject, and close-clears behavior plus circular checkbox styling.
- `widgets/tests/test_save_settings_dialog.cpp`: offscreen Qt dialog interaction tests.
- `widgets/tests/save_settings_dialog_tests.pro`: isolated Qt Widgets test target.

**Modify:**

- `pipeline/PipelineContext.h`: add output base directory, save switches, and timestamp to `HoloConfig`.
- `pipeline/PipelineConfig.cpp`: parse new fields with all save switches defaulting off.
- `pipeline/HoloPipeline.h`: accept an optional `ResultSaveReport` output beside `ElementalMemoryResult`.
- `pipeline/HoloPipeline.cpp`: call best-effort writers after mesh, multiview, and elemental stages and preserve the memory chain.
- `pipeline/PipelineLogger.h` and `pipeline/PipelineLogger.cpp`: record persistence selection, output directories, and warnings.
- `pipeline/PipelineModule.pri`: compile and expose the new persistence module.
- `config/ui_pipeline_template.ini`: add placeholders for switches and one run timestamp.
- `ui/CaptureWindow.ui`: place “保存设置” immediately after “打印配置”.
- `widgets/CaptureWindow.h` and `widgets/CaptureWindow.cpp`: own session settings, open the dialog, freeze the run timestamp, stop deleting historical result directories, pass the report across the worker thread, and display warnings after the print dialog closes.
- `mergeholo.pro`: compile the new dialog and form.

---

### Task 1: Shared Save Contract and Config Parsing

**Files:**

- Create: `pipeline/ResultSaveSettings.h`
- Create: `pipeline/tests/test_result_persistence.cpp`
- Create: `pipeline/tests/result_persistence_tests.pro`
- Modify: `pipeline/PipelineContext.h`
- Modify: `pipeline/PipelineConfig.cpp`

**Interfaces:**

- Produces: `ResultSaveSettings { bool mesh; bool multiview; bool elemental; }` with all members defaulting to `false`.
- Produces: `ResultSaveWarning { std::string resultType; std::filesystem::path outputDirectory; std::string message; }`.
- Produces: `ResultSaveReport::addWarning(...)`, `hasWarnings() const`, `clear()`, and public read-only warning access.
- Produces `HoloConfig::meshOutDir`, `saveSettings`, and `resultTimestamp` for later tasks.

- [ ] **Step 1: Write failing config and report tests**

Add a console test harness with explicit `expect(...)` failures. Write temporary INI files and assert:

```cpp
void testDefaultsAreMemoryOnly()
{
    const fs::path ini = writeIni(
        "output_root=output\n"
        "multiview_out_dir=multiview\n"
        "elemental_out_dir=elemental\n");
    HoloConfig config;
    applyConfig(config, ini);
    expect(!config.saveSettings.mesh, "mesh persistence must default off");
    expect(!config.saveSettings.multiview, "multiview persistence must default off");
    expect(!config.saveSettings.elemental, "elemental persistence must default off");
}

void testExplicitCombinationAndTimestamp()
{
    const fs::path ini = writeIni(
        "output_root=output\n"
        "mesh_out_dir=mesh\n"
        "save_mesh_result=true\n"
        "save_multiview_result=false\n"
        "save_elemental_result=true\n"
        "result_timestamp=20260716_153045_123\n");
    HoloConfig config;
    applyConfig(config, ini);
    expect(config.saveSettings.mesh, "mesh flag was not parsed");
    expect(!config.saveSettings.multiview, "multiview flag was not parsed");
    expect(config.saveSettings.elemental, "elemental flag was not parsed");
    expect(config.resultTimestamp == "20260716_153045_123", "timestamp was not parsed");
}
```

Also test `ResultSaveReport::addWarning`, `hasWarnings`, and `clear` without Qt, OpenCV, or PCL.

- [ ] **Step 2: Add the isolated qmake test target and run red**

`result_persistence_tests.pro` must compile the test plus `PipelineConfig.cpp`, include `pipeline`, `pipeline/elemental`, `pipeline/multiview`, `pipeline/stages`, and `vendor/multiview`, and include the existing `Pri/holo_pipeline.pri` for OSG headers.

Run from `C:\wzp\Holographicface\mergeholo\build\result_persistence_tests`:

```powershell
cmd /d /c 'call "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && "C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe" "..\..\pipeline\tests\result_persistence_tests.pro" "CONFIG+=release" -o Makefile && nmake /NOLOGO /f Makefile'
```

Expected: compile failure because `ResultSaveSettings`, the warning report, and new `HoloConfig` fields do not exist.

- [ ] **Step 3: Implement the minimal shared contract**

Create `ResultSaveSettings.h` with plain C++ value types. Keep warning storage private and expose:

```cpp
class ResultSaveReport {
public:
    void addWarning(std::string resultType,
                    std::filesystem::path outputDirectory,
                    std::string message);
    bool hasWarnings() const;
    const std::vector<ResultSaveWarning>& warnings() const;
    void clear();
private:
    std::vector<ResultSaveWarning> warnings_;
};
```

Inline these small methods so the config-only test has no extra link dependency.

- [ ] **Step 4: Extend and parse `HoloConfig`**

Add:

```cpp
std::filesystem::path meshOutDir;
ResultSaveSettings saveSettings;
std::string resultTimestamp;
```

Resolve `mesh_out_dir` relative to `output_root`, matching existing multiview and elemental behavior. Parse exactly `save_mesh_result`, `save_multiview_result`, `save_elemental_result`, and `result_timestamp`. Missing keys must preserve false/empty defaults.

- [ ] **Step 5: Run green and commit**

Run the qmake/nmake command above, then:

```powershell
.\release\result_persistence_tests.exe
```

Expected: exit code `0` and a PASS summary covering defaults, explicit combinations, paths, timestamp, and report clearing.

Commit only Task 1 paths:

```powershell
git add -- mergeholo/pipeline/ResultSaveSettings.h mergeholo/pipeline/PipelineContext.h mergeholo/pipeline/PipelineConfig.cpp mergeholo/pipeline/tests/test_result_persistence.cpp mergeholo/pipeline/tests/result_persistence_tests.pro
git commit -m "test: define processing result save contract"
```

---

### Task 2: Best-Effort Writers and Pipeline Hooks

**Files:**

- Create: `pipeline/ResultPersistence.h`
- Create: `pipeline/ResultPersistence.cpp`
- Modify: `pipeline/tests/test_result_persistence.cpp`
- Modify: `pipeline/tests/result_persistence_tests.pro`
- Modify: `pipeline/HoloPipeline.h`
- Modify: `pipeline/HoloPipeline.cpp`
- Modify: `pipeline/PipelineLogger.h`
- Modify: `pipeline/PipelineLogger.cpp`
- Modify: `pipeline/PipelineModule.pri`

**Interfaces:**

- Consumes: `HoloConfig::saveSettings`, output base directories, `resultTimestamp`, and `ResultSaveReport` from Task 1.
- Produces: `timestampedResultDirectory(baseDir, timestamp)`.
- Produces: `persistMeshResult`, `persistMultiviewResult`, and `persistElementalResult`; each returns `void`, appends warnings on failure, and never throws.
- Produces: `runHoloPipelineCliWithResult(argc, argv, elementalResult, saveReport)` while retaining the existing three-argument overload for callers that do not need warnings.

- [ ] **Step 1: Add failing path and synthetic writer tests**

Extend the test harness to cover:

```cpp
expect(timestampedResultDirectory("output/mesh", "20260716_153045_123")
       == fs::path("output/mesh_20260716_153045_123"),
       "timestamp suffix was not applied");
```

Build a one-triangle `pcl::PolygonMesh`, persist it, reload it with `pcl::io::loadPLYFile`, and assert nonempty cloud and one polygon.

Build a 2-by-2 multiview plan with 2-by-2 RGB frames whose top and bottom rows use distinct colors. Persist it and assert:

- four files named `0101.jpg`, `0102.jpg`, `0201.jpg`, and `0202.jpg` when `viewNameDigits=2`;
- each decoded JPEG is 2-by-2;
- decoded top/bottom and red/blue channel dominance matches the legacy vertical flip and RGB-to-BGR conversion.

Build a 2-by-2 `ElementalMemoryResult` with four 2-by-2 RGB images and assert `11.jpg...22.jpg`, dimensions, and channel dominance.

Pass a base path whose parent is a regular file and assert persistence returns normally, `report.hasWarnings()` is true, and the warning names the correct result type and output directory.

- [ ] **Step 2: Run red**

Extend `result_persistence_tests.pro` to include `Pri/common.pri`, `Pri/opencv.pri`, and `Pri/holo_pipeline.pri`, plus the existing PCL/OSG include roots used by the application. Add `ResultPersistence.cpp`, `memoryAtlasPageSink.cpp`, `memoryFrameSink.cpp`, `multiviewAtlasPlan.cpp`, and `multiviewRenderPlan.cpp` to the test sources. Regenerate and build `result_persistence_tests`.

Expected: compile failure because `ResultPersistence.h` and writer functions do not exist.

- [ ] **Step 3: Implement timestamped writers**

Implement one internal warning boundary per writer:

```cpp
try {
    fs::create_directories(outputDirectory);
    // encode all files
}
catch (const std::exception& ex) {
    report.addWarning(kind, outputDirectory, ex.what());
}
```

If an encoder returns failure without throwing, add one warning and stop writing that result type. Keep all files already written.

Use `pcl::io::savePLYFile` for `0_mesh.ply`. For multiview, derive one-based row/column from `MultiviewRenderPlan::frameAt`, flip raw OpenGL bottom-up rows when `captureFlipVertical=true`, convert RGB to OpenCV BGR, and use `IMWRITE_JPEG_QUALITY`. For elemental, wrap each materialized RGB image, convert to BGR without an additional vertical flip, and use decimal digits derived independently from target rows and target columns.

- [ ] **Step 4: Hook writers into the memory pipeline**

In `runPipeline`, create/use the caller-provided report and call:

```cpp
if (config.saveSettings.mesh && meshMemory.hasMesh()) {
    persistMeshResult(config, meshMemory, saveReport);
}
```

immediately after mesh success; do the equivalent after multiview success and elemental success. Do not use writer success as a stage return value. Ensure `useMeshMemory` is true whenever mesh persistence needs an in-memory result.

Keep the existing `runHoloPipelineCliWithResult(int, char**, ElementalMemoryResult*)` overload and forward it to the new four-argument overload with `nullptr` report, preserving printing and CLI source compatibility.

- [ ] **Step 5: Add report logging**

Extend `writePipelineLog` with `const ResultSaveReport&`. Add a `[result_persistence]` section containing selected booleans, timestamp, resolved timestamped directories, warning count, and indexed warning fields. Preserve current stage and memory sections.

- [ ] **Step 6: Run green and commit**

Build and run `result_persistence_tests`; expected exit `0` with all config, PLY, JPEG, orientation, naming, and failure tests passing.

Commit only Task 2 paths:

```powershell
git add -- mergeholo/pipeline/ResultPersistence.h mergeholo/pipeline/ResultPersistence.cpp mergeholo/pipeline/tests/test_result_persistence.cpp mergeholo/pipeline/tests/result_persistence_tests.pro mergeholo/pipeline/HoloPipeline.h mergeholo/pipeline/HoloPipeline.cpp mergeholo/pipeline/PipelineLogger.h mergeholo/pipeline/PipelineLogger.cpp mergeholo/pipeline/PipelineModule.pri
git commit -m "feat: persist selected pipeline results"
```

---

### Task 3: Save Settings Dialog

**Files:**

- Create: `ui/SaveSettingsDialog.ui`
- Create: `widgets/SaveSettingsDialog.h`
- Create: `widgets/SaveSettingsDialog.cpp`
- Create: `widgets/tests/test_save_settings_dialog.cpp`
- Create: `widgets/tests/save_settings_dialog_tests.pro`

**Interfaces:**

- Consumes: `ResultSaveSettings` from Task 1.
- Produces: `SaveSettingsDialog::setSaveSettings(const ResultSaveSettings&)` and `saveSettings() const`.
- Guarantees: confirm accepts current independent combination; cancel, custom close, system close, and `reject()` all return an all-false value.

- [ ] **Step 1: Write failing offscreen dialog tests**

Create a `QApplication` console test with explicit `expect(...)` and `findChild` calls. Assert:

```cpp
SaveSettingsDialog dialog;
expect(!dialog.saveSettings().mesh, "mesh must default off");

ResultSaveSettings selected;
selected.mesh = true;
selected.elemental = true;
dialog.setSaveSettings(selected);
expect(dialog.saveSettings().mesh && !dialog.saveSettings().multiview
       && dialog.saveSettings().elemental,
       "dialog did not preserve independent selections");

dialog.findChild<QPushButton*>("cancelButton")->click();
expect(!dialog.saveSettings().mesh
       && !dialog.saveSettings().multiview
       && !dialog.saveSettings().elemental,
       "cancel must clear all settings");
```

Repeat for `closeButton` and a direct close event. Confirm that `confirmButton` preserves and accepts the selected combination.

- [ ] **Step 2: Add the dialog qmake target and run red**

The `.pro` target includes Qt Core/Gui/Widgets, `SaveSettingsDialog.cpp`, `SaveSettingsDialog.ui`, and `ResultSaveSettings.h`. Build in `build/save_settings_dialog_tests` with qmake/nmake and run with:

```powershell
$env:QT_QPA_PLATFORM='offscreen'; .\release\save_settings_dialog_tests.exe
```

Expected: compile failure because the dialog does not exist.

- [ ] **Step 3: Implement the reference dialog**

Create a fixed, compact dialog matching the supplied reference: three vertically spaced `QCheckBox` controls, a top-right `QToolButton`, and bottom confirm/cancel buttons. Use `QStyle::SP_TitleBarCloseButton` for the close icon and a tooltip of `关闭`; use circular checkbox indicators while retaining true checkbox semantics and arbitrary multi-selection.

Connect confirm to `accept()`. Override `reject()` and `closeEvent()` through one `clearSettingsAndReject()` path so every non-confirm close clears all checkboxes before the dialog exits.

- [ ] **Step 4: Run green and commit**

Build and run the offscreen dialog tests. Expected: exit `0`, with default, independent combination, confirm, cancel, custom close, and system close cases passing.

Commit only the new dialog and test files:

```powershell
git add -- mergeholo/ui/SaveSettingsDialog.ui mergeholo/widgets/SaveSettingsDialog.h mergeholo/widgets/SaveSettingsDialog.cpp mergeholo/widgets/tests/test_save_settings_dialog.cpp mergeholo/widgets/tests/save_settings_dialog_tests.pro
git commit -m "feat: add processing save settings dialog"
```

---

### Task 4: CaptureWindow Session and Warning Integration

**Files:**

- Modify: `config/ui_pipeline_template.ini`
- Modify: `ui/CaptureWindow.ui`
- Modify: `widgets/CaptureWindow.h`
- Modify: `widgets/CaptureWindow.cpp`
- Modify: `mergeholo.pro`
- Test: `pipeline/tests/test_result_persistence.cpp`
- Test: `widgets/tests/test_save_settings_dialog.cpp`

**Interfaces:**

- Consumes: `SaveSettingsDialog`, `ResultSaveSettings`, the four-argument pipeline API, and `ResultSaveReport`.
- Produces: session-only settings, one frozen run timestamp, generated INI values, no historical result-directory deletion, and one post-print warning summary.

- [ ] **Step 1: Add failing generated-config coverage**

Extend the config test fixture to load text equivalent to the UI template replacements and assert the three independent booleans plus timestamp. Add a source-level UI check in the dialog test that `CaptureWindow.ui` contains `saveSettingsButton` immediately after `printSettingsButton` in the button layout.

Run both test executables. Expected: failure because template placeholders and the main-window button are absent.

- [ ] **Step 2: Add the main-window button and session state**

Add `saveSettingsButton` after `printSettingsButton`, with the same `138x56` minimum size and text `保存设置`. Add it to the existing shared button style list.

Add members:

```cpp
QPushButton* saveSettingsButton_ = nullptr;
ResultSaveSettings saveSettings_;
QString activeResultTimestamp_;
std::shared_ptr<ResultSaveReport> resultSaveReport_;
```

`openSaveSettings()` loads `saveSettings_` into a modal dialog. On `Accepted`, replace `saveSettings_` with the dialog value. On every rejected/closed result, replace it with the dialog's guaranteed all-false value. Disable the button only while state is `Processing` or `Starting`.

- [ ] **Step 3: Freeze and write the run snapshot**

At the start of `startProcessing()`, before preparing input, set:

```cpp
activeResultTimestamp_ = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
```

Add exact template keys:

```ini
mesh_out_dir=mesh
save_mesh_result={{save_mesh_result}}
save_multiview_result={{save_multiview_result}}
save_elemental_result={{save_elemental_result}}
result_timestamp={{result_timestamp}}
```

Replace each placeholder using lowercase `true`/`false` and the frozen timestamp. In `preparePipelineInput`, remove only the existing `output/input` directory; delete the current removal of static `multiview` and `elemental` directories, and do not add any result-directory cleanup.

- [ ] **Step 4: Carry warnings across the worker thread without blocking print**

Allocate a fresh `ResultSaveReport` beside `ElementalMemoryResult` in the pipeline worker and pass it to the four-argument API. After the worker finishes successfully, publish both shared results under the existing mutex before the UI thread reads them.

On success:

1. set the Done state;
2. open `Print9030Dialog` with the elemental memory result exactly as today;
3. only after `openPrintSettings()` returns, show one `QMessageBox::warning` if the report contains warnings;
4. include each warning's result type, native output directory, and message;
5. clear the consumed report so manually reopening print settings does not repeat it.

Do not change the success exit code or skip the print dialog because warnings exist.

- [ ] **Step 5: Register sources/forms and run focused tests**

Add the new dialog `.cpp`, `.h`, and `.ui` to `mergeholo.pro` without disturbing the existing uncommitted printing entries.

Run `result_persistence_tests` and `save_settings_dialog_tests`. Expected: both exit `0`.

- [ ] **Step 6: Commit the integration paths only**

Review `git diff` carefully because `CaptureWindow`, `CaptureWindow.ui`, and `mergeholo.pro` already contain user changes. Stage only the intended hunks/files without reverting unrelated content, then commit:

```powershell
git commit -m "feat: connect save settings to capture processing"
```

---

### Task 5: Full Verification and Final Review

**Files:**

- Verify: all Task 1-4 paths
- Modify only if verification exposes a defect covered by this specification.

**Interfaces:**

- Consumes all previous task outputs.
- Produces fresh test/build evidence and a reviewer-approved implementation.

- [ ] **Step 1: Run all focused automated tests**

Run freshly:

```powershell
build\result_persistence_tests\release\result_persistence_tests.exe
$env:QT_QPA_PLATFORM='offscreen'; build\save_settings_dialog_tests\release\save_settings_dialog_tests.exe
build\printing_tests\release\printing_tests.exe
00-bin\elemental_tests\test_elemental_current.exe
```

Expected: every executable exits `0` and prints no failed checks.

- [ ] **Step 2: Run Release build**

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Config release -SkipDeploy
```

Expected: qmake, compilation, and link complete with exit code `0` and produce `00-bin\mergeholo.exe`.

- [ ] **Step 3: Run reduced integration persistence checks**

Create a temporary reduced configuration under `output/test-result-persistence` using a small existing input pair and settings `multiview_angle=2`, `multiview_per=1`, `multiview_resolution=4`, `target_rows=2`, `target_cols=2`, all three save switches true, and a fixed test timestamp. Run the CLI pipeline and verify:

- `mesh_<timestamp>/0_mesh.ply` exists and reloads;
- `multiview_<timestamp>` contains 4 JPEGs named `001001.jpg...002002.jpg`, each 4-by-4;
- `elemental_<timestamp>` contains 4 JPEGs named `11.jpg...22.jpg`, each 2-by-2;
- the pipeline log reports zero persistence warnings.

If the graphics runtime cannot execute headlessly, record that limitation and retain the synthetic writer tests plus Release build as evidence; do not claim the reduced integration run passed.

- [ ] **Step 4: Verify best-effort failure does not block printing data**

Run the synthetic invalid-path test and the printing module tests freshly. Confirm the former records a warning without throwing and the latter still consumes a materialized `ElementalMemoryResult` successfully.

- [ ] **Step 5: Inspect UI and conduct final code review**

Launch the Release executable, inspect the new button and dialog against both reference images, and verify text does not overlap at the current window size. Review the complete feature diff against `docs/superpowers/specs/2026-07-16-processing-result-save-settings-design.md`; fix all Critical or Important findings and rerun the covering tests.

- [ ] **Step 6: Report evidence**

Summarize changed behavior, exact output directory examples, automated test counts, Release build result, reduced integration result or limitation, and any hardware not exercised. Do not stage or commit unrelated workspace changes.

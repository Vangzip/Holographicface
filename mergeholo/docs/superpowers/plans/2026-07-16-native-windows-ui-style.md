# MergeHolo Native Windows UI Style Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Unify the capture window, save-settings dialog, and print-configuration dialog around the Windows-native Qt Widgets style while preserving all processing and hardware behavior.

**Architecture:** A focused `NativeUiStyle` function selects Qt's `windowsvista` style when available and sets the application font once at startup. Window-specific sizing remains in the existing `.ui` forms; heavy custom QSS, frameless chrome, and custom checkbox painting are removed. Existing dialog behavior tests are extended with native-style and source-form assertions before implementation.

**Tech Stack:** Qt 5.15 Widgets, C++17, qmake, MSVC 2019 v142 x64, offscreen Qt tests, Windows desktop screenshot review.

## Global Constraints

- Use Qt Widgets and the Windows-native style; do not add Qt Quick, QML, Fluent libraries, or image assets.
- Remove thick black borders, frameless dialog chrome, custom close controls, and circular checkbox painting.
- Use `Microsoft YaHei UI` at a fixed desktop point size and retain native focus, hover, pressed, and disabled states.
- Preserve capture, processing, result persistence, print runner, D3D11/VSync, second-screen selection, exposure, motion, cancellation, error, and homing behavior.
- Preserve save-dialog semantics: accept keeps the selected combination; Cancel, system close, Escape, and `reject()` clear all selections.
- Do not revert, clean, stage, or commit unrelated worktree changes.

---

## File Structure

**Create:**

- `widgets/NativeUiStyle.h`: declares the single application-style entry point.
- `widgets/NativeUiStyle.cpp`: selects `windowsvista` with fallback and sets the application font.

**Modify:**

- `apps/mergeholo_main.cpp`: applies the native style immediately after constructing `QApplication`.
- `mergeholo.pro`: registers the new style source and header without disturbing printing entries.
- `ui/CaptureWindow.ui`: removes thick custom borders and uses stable native button dimensions.
- `widgets/CaptureWindow.cpp`: removes per-button QSS.
- `ui/SaveSettingsDialog.ui`: restores native chrome, standard checkboxes, and `QDialogButtonBox`.
- `widgets/SaveSettingsDialog.cpp`: removes the custom checkbox style and connects the standard button box.
- `ui/Print9030Dialog.ui`: standardizes margins, spacing, and command-button dimensions.
- `widgets/tests/test_save_settings_dialog.cpp`: tests style application, native save-dialog chrome, rejection behavior, and form-level style constraints.
- `widgets/tests/save_settings_dialog_tests.pro`: compiles the application-style helper into the focused test.

---

### Task 1: Native Style Contract and Application Entry

**Files:**

- Create: `widgets/NativeUiStyle.h`
- Create: `widgets/NativeUiStyle.cpp`
- Modify: `apps/mergeholo_main.cpp`
- Modify: `mergeholo.pro`
- Modify: `widgets/tests/test_save_settings_dialog.cpp`
- Modify: `widgets/tests/save_settings_dialog_tests.pro`

**Interfaces:**

- Produces: `void applyNativeWindowsUiStyle(QApplication& application)`.
- Guarantees: requests `windowsvista` only when available, always sets `Microsoft YaHei UI` at 9 points, and never prevents startup when the style plugin is absent.

- [ ] **Step 1: Add a failing application-style test**

Include `NativeUiStyle.h` and add:

```cpp
void testNativeApplicationStyle(QApplication& application)
{
    applyNativeWindowsUiStyle(application);
    expect(application.font().family() == "Microsoft YaHei UI",
        "application font must use Microsoft YaHei UI");
    expect(application.font().pointSize() == 9,
        "application font must use the fixed desktop point size");
}
```

Call it before the dialog tests and add `../NativeUiStyle.cpp`/`.h` to the test `.pro` file.

- [ ] **Step 2: Run red**

Regenerate and build `build/save_settings_dialog_tests`, then run with `QT_QPA_PLATFORM=offscreen`.

Expected: compile failure because `NativeUiStyle.h` does not exist.

- [ ] **Step 3: Implement the style helper**

```cpp
// NativeUiStyle.h
#pragma once
class QApplication;
void applyNativeWindowsUiStyle(QApplication& application);
```

```cpp
// NativeUiStyle.cpp
#include "NativeUiStyle.h"
#include <QApplication>
#include <QFont>
#include <QStyleFactory>

void applyNativeWindowsUiStyle(QApplication& application)
{
#ifdef Q_OS_WIN
    if (QStyle* style = QStyleFactory::create("windowsvista")) {
        application.setStyle(style);
    }
#endif
    QFont font("Microsoft YaHei UI");
    font.setPointSize(9);
    application.setFont(font);
}
```

Call it immediately after `QApplication app(argc, argv);` and register both files in `mergeholo.pro`.

- [ ] **Step 4: Run green**

Build and run the focused dialog tests. Expected: exit `0` with the existing behavior tests and the font assertions passing.

---

### Task 2: Native Save-Settings Dialog

**Files:**

- Modify: `ui/SaveSettingsDialog.ui`
- Modify: `widgets/SaveSettingsDialog.cpp`
- Modify: `widgets/tests/test_save_settings_dialog.cpp`

**Interfaces:**

- Consumes: existing `setSaveSettings`, `saveSettings`, `reject`, and `closeEvent` APIs.
- Produces: a standard modal dialog with `QDialogButtonBox` named `buttonBox`, standard `Ok`/`Cancel` buttons, and unchanged rejection semantics.

- [ ] **Step 1: Add failing native-dialog assertions**

```cpp
void testUsesNativeDialogControls()
{
    SaveSettingsDialog dialog;
    expect(!dialog.windowFlags().testFlag(Qt::FramelessWindowHint),
        "save settings must use native window chrome");
    expect(dialog.findChild<QToolButton*>("closeButton") == nullptr,
        "save settings must not contain a custom close button");
    expect(dialog.findChild<QDialogButtonBox*>("buttonBox") != nullptr,
        "save settings must use a standard dialog button box");
    expect(dialog.findChild<QCheckBox*>("meshCheckBox")->style() == QApplication::style(),
        "save settings must use the application checkbox style");
}
```

Update accept/cancel interaction tests to click `buttonBox->button(QDialogButtonBox::Ok)` and `Cancel`.

- [ ] **Step 2: Run red**

Run the focused test. Expected: failure on frameless chrome/custom close/button-box assertions.

- [ ] **Step 3: Replace the custom form**

Use a 420x270 resizable `QDialog` with 16px outer margins, a `QGroupBox` titled `保存内容`, three standard checkboxes, a vertical spacer, and a right-aligned `QDialogButtonBox` with `Ok|Cancel`. Remove the form stylesheet and in-content close button.

Remove `CircularCheckBoxStyle`, `QPainter`, `QProxyStyle`, and the frameless-window flag. Connect:

```cpp
connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &SaveSettingsDialog::reject);
```

Set the standard button text to `确定` and `取消` after `setupUi`.

- [ ] **Step 4: Run green**

Run offscreen tests. Expected: default, arbitrary combination, accept, Cancel, system close, Escape/direct reject, and native-control checks all pass.

---

### Task 3: Capture and Print Form Harmonization

**Files:**

- Modify: `ui/CaptureWindow.ui`
- Modify: `widgets/CaptureWindow.cpp`
- Modify: `ui/Print9030Dialog.ui`
- Modify: `widgets/tests/test_save_settings_dialog.cpp`

**Interfaces:**

- Consumes: application-level native style from Task 1.
- Produces: standard frame/progress/button rendering with stable form dimensions and unchanged object names/signals.

- [ ] **Step 1: Add failing form assertions**

Read the forms relative to `QCoreApplication::applicationDirPath()` and assert:

```cpp
expect(!captureUi.contains("border: 2px solid #111111"),
    "capture form must not contain thick black borders");
expect(!captureUi.contains("QProgressBar::chunk"),
    "capture form must use the native progress bar");
expect(!captureCpp.contains("button->setStyleSheet"),
    "capture buttons must use the application style");
expect(!printUi.contains("<property name=\"styleSheet\">"),
    "print form must remain on the native application style");
```

Also verify all five capture command buttons are present and the save button still immediately follows print settings.

- [ ] **Step 2: Run red**

Run the focused test. Expected: failure on the capture border/progress/button-style assertions.

- [ ] **Step 3: Update the capture form**

Remove label and progress-bar QSS. Set preview labels to `QFrame::StyledPanel` with `QFrame::Sunken`, line width 1. Set button layout spacing to 12 and all five buttons to minimum 112x36. Delete the `button->setStyleSheet(...)` call while preserving the existing button list and signal connections.

- [ ] **Step 4: Update the print form**

Set the root layout margins to 16 and spacing to 12. Set `browseFolderButton` to at least 88x32 and `startButton`, `cancelButton`, and `closeButton` to at least 88x32. Keep every object name, field, tab, and enable-state behavior unchanged.

- [ ] **Step 5: Run green**

Build and run the focused dialog test. Expected: all native-form and behavior assertions pass.

---

### Task 4: Full Regression and Visual Review

**Files:**

- Verify: all Task 1-3 paths.
- Modify only if a verification failure is caused by this style change.

**Interfaces:**

- Consumes: final native-style forms and unchanged functional modules.
- Produces: fresh automated/build evidence and screenshots at Windows scale factors 1.0 and 1.25.

- [ ] **Step 1: Run focused tests**

Run freshly:

```powershell
$env:QT_QPA_PLATFORM='offscreen'; .\build\save_settings_dialog_tests\release\save_settings_dialog_tests.exe
.\build\result_persistence_tests\release\result_persistence_tests.exe
.\FF-tmp\printing-tests\release\printing_tests.exe
.\00-bin\elemental_tests\test_elemental_current.exe
```

Expected: every process exits `0` and prints its pass summary; the intentional invalid persistence path may print one warning.

- [ ] **Step 2: Run the Release build**

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Config release -SkipDeploy
```

Expected: exit `0` and `00-bin/mergeholo.exe` exists.

- [ ] **Step 3: Capture and inspect windows**

Launch the Release UI with `QT_SCALE_FACTOR=1` and `1.25`. Capture the main window, open and capture Save Settings, then open and capture Print Configuration. Confirm system title bars, native square checkboxes, no thick black borders, consistent fonts/buttons, and no overlap or clipped text.

- [ ] **Step 4: Review the complete feature diff**

Run `git diff --check` on the touched paths, inspect that no printing/control-card methods changed, and conduct a read-only review for Critical/Important regressions.

- [ ] **Step 5: Commit only isolated style paths**

Stage new helper/test/form paths only when they do not contain unrelated changes. Leave `mergeholo.pro`, `CaptureWindow.*`, and any overlapping 9030 work unstaged if an isolated commit cannot be made without including existing user changes.


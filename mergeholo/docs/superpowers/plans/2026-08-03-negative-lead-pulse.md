# Negative Reverse Lead Pulse Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Allow `lead_pulse` (`反向提前`) to be negative without changing the strict current V2 timing formula or any IMC60G/SV660N hardware behavior.

**Architecture:** Keep the signed value flowing unchanged through the existing Qt spin box and `Print9030Config`. Separate validation in `buildV2PrintPlan`: `addTempPulse` remains non-negative while `leadPulse` is accepted as signed input and processed by the existing checked subtraction. Lock the behavior at the timing boundary and dialog/config round-trip boundary.

**Tech Stack:** C++17, Qt 5.15 Widgets/Core, qmake, MSVC 2019 x64, PowerShell verification scripts.

## Global Constraints

- `add_temp_pulse` remains non-negative.
- `lead_pulse` accepts `-2147483647` through `2147483647` pulse and is never clamped or sign-changed.
- Preserve `reverseDelayPulse = addTempPulse - reverseFixedPulse - forwardDelayPulse - leadPulse` exactly.
- Preserve qint64 checked arithmetic, int32 SDK-position checks, VBlank validation, and fail-closed behavior.
- Do not alter motion targets, exposure comparison positions, Card0, X→Axis1, Y→Axis0, homing, EtherCAT, or SV660N DO1 behavior.
- Automated tests and diagnostics must remain hardware-silent.
- Preserve all unrelated dirty-worktree files, especially `config/print_9030.ini`; tests must use temporary configuration roots.

---

### Task 1: Signed V2 reverse lead timing

**Files:**
- Modify: `printing/tests/test_v2_print_timing.cpp:94-127,196-244`
- Modify: `printing/V2PrintTiming.cpp:183-194`

**Interfaces:**
- Consumes: `V2PrintPlan buildV2PrintPlan(const Print9030Config&, const PrintHardwareProfile&, double, QString*)`.
- Produces: the existing interface with `PrintMainConfig::leadPulse` accepted as signed input; no new production API.

- [ ] **Step 1: Add failing timing and validation regressions**

In `testProfileConstantsAndReverseDelayBoundaries()`, add the approved现场 value:

```cpp
    config.main.addTempPulse = 16000;
    config.main.leadPulse = -5000;
    plan = build(config, 60.0, &error);
    expect(error.isEmpty() && plan.rows[1].startDelayFrames == 15,
        "negative lead pulse must be subtracted as signed input: " + error);
```

In `testInvalidConfigurationFailsClosed()`, preserve the independent `addTempPulse` safety boundary:

```cpp
    config = defaultPrint9030Config();
    config.main.addTempPulse = -1;
    expectInvalid(config, 60.0, "addTempPulse");
```

- [ ] **Step 2: Run the focused timing test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_qmake_test.ps1 -Project printing/tests/v2_print_timing_tests.pro
```

Expected: FAIL because `leadPulse=-5000` is rejected by the current combined non-negative validation. The negative `addTempPulse` assertion must already pass.

- [ ] **Step 3: Split the validation with the minimal production change**

Replace the combined check in `printing/V2PrintTiming.cpp` with:

```cpp
    if (config.main.addTempPulse < 0) {
        return fail(errorMessage,
            QString("addTempPulse must be non-negative; value=%1")
                .arg(config.main.addTempPulse));
    }
```

Do not modify the existing checked `leadPulse` subtraction or delay-frame conversion.

- [ ] **Step 4: Run the focused timing test and verify GREEN**

Run the same `run_qmake_test.ps1` command.

Expected: `v2_print_timing_tests: PASS`, including 15 reverse start-delay frames for `16000,-5000`.

- [ ] **Step 5: Commit Task 1 only**

```powershell
git add -- printing/V2PrintTiming.cpp printing/tests/test_v2_print_timing.cpp
git commit -m "fix: accept signed reverse lead pulse"
```

---

### Task 2: Dialog/config round trip and operator contract

**Files:**
- Modify: `widgets/tests/test_print9030_dialog.cpp:191-239`
- Modify: `docs/imc60g-print-operator.md:81-91`

**Interfaces:**
- Consumes: `Print9030Dialog`, `Print9030Config`, `loadPrint9030Config()` and the existing `leadPulseSpin` signed range.
- Produces: regression evidence that `-5000` survives UI snapshot, save, and reload; updated operator documentation.

- [ ] **Step 1: Add a UI/config round-trip regression**

Change the existing retained-field test to enter a negative value:

```cpp
    dialog.findChild<QSpinBox*>("leadPulseSpin")->setValue(-5000);
```

Update the start snapshot assertion:

```cpp
            && controller.startedConfig.main.leadPulse == -5000,
```

Extend the persisted-config assertion:

```cpp
    expect(loadError.isEmpty() && persisted.main.gridRows == 1
            && persisted.main.gridColumns == 1
            && persisted.main.leadPulse == -5000,
        "negative lead pulse must persist unchanged in print_9030.ini");
```

- [ ] **Step 2: Run the focused dialog test and verify the boundary**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_qmake_test.ps1 -Project widgets/tests/print9030_dialog_tests.pro
```

Expected before any UI production change: PASS. This proves the existing signed spin-box range and config serialization already meet the requirement. If it fails, stop and diagnose rather than widening scope speculatively.

- [ ] **Step 3: Update the operator contract**

Replace the `反向提前` table description in `docs/imc60g-print-operator.md` with:

```markdown
| 反向提前 | pulse | `lead_pulse`；有符号反向显示延迟修正量，范围为 `-2147483647～2147483647`。按 `addTempPulse - reverseFixedPulse - forwardDelayPulse - leadPulse` 计算，因此负值会增加反向显示延迟；仍须形成无溢出且有效的 VBlank 计划 |
```

- [ ] **Step 4: Re-run both focused tests**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_qmake_test.ps1 -Project printing/tests/v2_print_timing_tests.pro
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_qmake_test.ps1 -Project widgets/tests/print9030_dialog_tests.pro
```

Expected: both test executables exit 0 and print PASS messages.

- [ ] **Step 5: Commit Task 2 only**

```powershell
git add -- widgets/tests/test_print9030_dialog.cpp docs/imc60g-print-operator.md
git commit -m "test: preserve negative reverse lead pulse"
```

---

### Task 3: Full hardware-silent verification and review

**Files:**
- Verify only: `scripts/test_printing.ps1`
- Verify only: `00-bin/mergeholo.exe`
- Verify only: `vendor/imc60g/bin/x64/IMC_Library_x64.dll`

**Interfaces:**
- Consumes: all Task 1 and Task 2 commits.
- Produces: complete software verification evidence and a reviewer verdict; no source edits unless review finds a concrete defect.

- [ ] **Step 1: Run the complete printing verification**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\test_printing.ps1
```

Expected: all seven printing/UI test projects PASS, the release x64 application builds, IMC60G SDK/runtime verification passes, print configurations deploy with matching hashes, obsolete x86/DFJZH DLLs are absent, UI Unicode binary checks pass, and diagnostics-only startup reports Card0 remains closed.

- [ ] **Step 2: Confirm scoped diff and hardware silence**

```powershell
git status --short
git diff HEAD~2 --check
git diff HEAD~2 --name-status
```

Expected source scope: `printing/V2PrintTiming.cpp`, `printing/tests/test_v2_print_timing.cpp`, `widgets/tests/test_print9030_dialog.cpp`, and `docs/imc60g-print-operator.md`, plus the already committed design/plan documents. No hardware acceptance evidence should be produced by these tests.

- [ ] **Step 3: Request independent cross-review**

Reviewer must verify:

```text
1. addTempPulse remains non-negative.
2. leadPulse=-5000 produces 15 reverse start-delay frames for active defaults.
3. The existing signed formula and overflow guards are unchanged.
4. UI snapshot and temporary INI reload preserve -5000 exactly.
5. No hardware API, motion, homing, or DO1 behavior changed.
```

Expected: PASS with no critical or important findings. Fix any valid finding with a new focused RED/GREEN cycle before final verification.

# 9030 Legacy Print Runner Design

## Status

Approved on 2026-07-14.

## Goal

Integrate the legacy 9030 print path into `PrintJobRunner` so that a job uses the
real motion card, the legacy second-screen selection rule, the legacy D3D11
presentation and VBlank path, and the legacy serpentine scan sequence.

The job must not start motion unless an external screen can render a warm-up frame
and wait for VBlank. It must never silently fall back to simulation if the control
card cannot initialize.

## Legacy Interfaces To Preserve

### Motion card

The controller loads `DfjzhControlerDll.dll` and uses board 0.

- `InitCard_ID(0, 1, 1, 1, 1, 0)` initializes four position-mode axes.
- `Home_ID(0, axis)` defines the current position as the logical zero for axes
  X, Y, Z, and W before the job starts. This is not physical homing.
- `SetAxisAcc_ID`, `SetAxisDec_ID`, `SetAxisVel_ID`, `SetAxisStartVel_ID`,
  `SetAxisStopVel_ID`, `SetAxisPos_ID`, and `StartAxis_ID` execute each move.
- `ReadAxisState_ID` reports whether an axis has stopped.
- `SetAxisIO_ID(0, 1, 6, 1, 1, 1)`, `Set_IO_Pos_ID`, and `Enable_IO_Pos_ID`
  configure the Y-axis position-comparison exposure output on O1.
- Exposure disarm is `Enable_IO_Pos_ID(0, 1, 0)`,
  `SetAxisIO_ID(0, 1, 6, 0, 1, 1)`, and `WriteIoBit_ID(0, 0, 1)`.

`IMotionController` will gain the current-position reset operation needed to map
to `Home_ID`. `DfjzhMotionController` must resolve it from the DLL. DLL loading,
symbol resolution, or `InitCard_ID` failure returns `false`; simulation is not a
valid production print mode.

### Second screen and VBlank

The screen presenter preserves the legacy Windows path.

1. Enumerate monitors with `EnumDisplayMonitors` and `MONITORINFOF_PRIMARY`.
2. Choose the final non-primary monitor encountered by the legacy enumeration.
3. Create the presentation window at that monitor rectangle.
4. Create the D3D11 device and two-buffer `IDXGISwapChain` for the window.
   Preserve the legacy hardware-device attempt and WARP fallback.
5. Upload BGR/BGRA pixels to a `DXGI_FORMAT_B8G8R8A8_UNORM` texture, draw the
   full-screen triangle, then call `Present(0, 0)`.
6. Obtain the containing `IDXGIOutput` and call `WaitForVBlank()`.

The original program falls back to half of the primary screen when no external
monitor exists, and its scan loop can fall back to `DwmFlush`. These fallbacks are
intentionally excluded: the agreed requirement is that a second screen and its
VBlank must be ready before the print starts.

The Qt presenter owns the window and D3D objects on the GUI thread. The worker
thread requests `prepare`, `present`, and `waitForVBlank` synchronously through a
thread-safe presenter boundary, so a frame is committed before the scan advances.

## Runner Sequence

### Preconditions

`PrintJobRunner` validates the image count, dimensions, active grid, positive
axis-Y timing values, and pulse calculations. It then prepares the second-screen
presenter, renders the first frame, and completes one VBlank wait. Any failure
finishes the job without initializing or moving the card.

After the display preflight succeeds, the runner initializes the card, resets the
logical positions of axes X/Y/Z/W with `Home_ID`, and forces O1 low.

### Legacy serpentine scan

The first row scans in the positive Y direction. Each following row switches Y
direction and reverses the order of the images in that row. Between rows X moves
one legacy row increment.

The active legacy loop calculates:

```text
stepPulse     = axisY.subdivision * axisY.resolution * abs(columnSpacingMm)
exposurePulse = stepPulse * gridColumns
accDistance   = round((startSpeed + speed) * ((speed - startSpeed) / acceleration) / 2)
totalPulse    = exposurePulse + 2 * accDistance + addTempPulse
```

For compatibility, the active legacy scan uses `columnSpacingMm` for both the
Y image pitch and the X row increment. `rowSpacingMm` is retained as configuration
but is not used by this active path.

For every row the runner:

1. Runs the legacy W-axis preparation move to logical position zero when the
   W electrical option is enabled, then waits for W to stop.
2. Computes the legacy Y constant-speed, exposure, and comparison positions,
   including the existing reverse-row `addTempPulse` and `stepPulse` adjustment.
3. Arms the Y-axis O1 position-comparison window using the active legacy
   forward/reverse boundary convention.
4. Starts one continuous Y move for `totalPulse`.
5. Reads the Y position through the constant-speed section. It chooses the next
   image from the same position-to-column calculation as the legacy loop, applies
   the legacy `leadPulse` display delay, presents it, and waits the calculated
   number of VBlank frames before the next image.
6. Waits for Y to stop, disarms the comparison output, forces O1 low, and, unless
   it was the final row, moves X by one row increment and waits for X to stop.

The comparison boundaries are explicit. For a positive-Y row,
`yConstBegin = yStart + accDistance`, `yExposeBegin = yConstBegin`, and the
legacy comparison window is `[yExposeBegin, LONG_MAX]`. For a reverse row,
`yConstBegin = yStart - accDistance`,
`yExposeBegin = yConstBegin - addTempPulse + stepPulse`, and the legacy
comparison window is `[LONG_MIN, yExposeBegin]`. The runner disarms this
sentinel-ended window after every Y move and before any X move, so O1 cannot
remain enabled during the deceleration, row-step, or cleanup phases.

The D3D presenter converts elemental-memory frames from the pipeline's contiguous
three-channel BGR storage and folder frames from decoded images into the D3D11
BGR/BGRA upload format. It does not write intermediate image files.

### Cleanup

One cleanup path handles completion, cancellation, and runtime errors:

1. Disarm the position-comparison output and force O1 low.
2. Stop any active axis.
3. Move X and Y back to logical zero with the configured legacy axis profiles and
   wait for both axes to stop.
4. Shut down the card and close the D3D presentation window.

Cleanup failures are appended to the original job failure. A completed job is not
reported successful unless the exposure is off and X/Y return-to-zero succeeds.

## User-visible Errors

The existing `Print9030Dialog` already shows a `QMessageBox::warning` for
`PrintJobRunner::finished(false, text)`. The runner will report all preflight,
card initialization, motion, exposure, image, VBlank, cancellation, and cleanup
errors through that signal. No error path may report success in simulation.

## Test Strategy

Add injected fake implementations of the motion controller and screen presenter
to the printing test target. Tests will verify:

- no card initialization or move occurs when second-screen preparation or VBlank
  fails;
- card initialization failure finishes unsuccessfully and starts no motion;
- one-row and multi-row jobs issue the expected serpentine movement, exposure,
  presentation, and VBlank order;
- cancellation and each injected runtime failure force O1 low and return X/Y to
  zero;
- unsupported image data fails before motion.

Build tests and the Windows Qt application without connected hardware. Then run a
manual hardware test with an external display and a small grid, confirming the
selected screen, VBlank preflight, O1 gating, image order, cancellation, and
return-to-zero behavior.

## Non-goals

- Physical limit-switch homing via `GoHome_ID` is not added; the legacy print path
  uses `Home_ID` logical zeroing.
- The scan geometry and timing are not redesigned. The active legacy formula is
  preserved, including `addTempPulse`, `leadPulse`, and reverse-row ordering.

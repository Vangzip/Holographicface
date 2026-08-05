# Recover camera input settings design

## Goal

Allow an operator to recover from an external-input selection and return to
live camera preview without leaving the settings button disabled.

## Behaviour

- When `input_mode` is an external mode, CaptureWindow enters `Frozen`, keeps
  the Settings button enabled, and shows that external input is active.
- A missing pre-initialized camera is reported as an initialization failure
  only when the selected input mode is Camera.
- Switching from an external input mode to Camera saves the new settings,
  launches a fresh MergeHolo process, and closes the current process.

## Rationale

The vendor GigE SDK must initialize before a QWidget is created. Reusing the
already-running GUI process would call initialization after the Qt event loop
is active and can hang. Relaunching lets startup initialize the device at the
known-good pre-window boundary.

## Error handling

- If the replacement process cannot be launched, leave the current window
  open and show an actionable error.
- If the fresh process cannot initialize the camera, it leaves Settings
  available so the operator can choose an external input or adjust settings.

## Verification

The settings test asserts that external input is not treated as camera failure
and that the entry point relaunches only after changing to camera input.

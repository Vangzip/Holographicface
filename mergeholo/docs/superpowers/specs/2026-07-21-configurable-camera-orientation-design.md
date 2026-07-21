# Configurable Camera Orientation Design

**Date:** 2026-07-21

## Goal

Correct the live RGB and depth images for the camera's new fixed inverted mounting while keeping RGB pixels, displayed depth, spatial depth, saved capture files, point-cloud geometry, and command-line capture aligned.

## Configuration

Add `capture_rotation=clockwise_90` to `config/default_camera.ini` and `CameraCaptureSettings`. Supported values are:

- `clockwise_90`: new fixed mounting and new default.
- `counterclockwise_90`: previous mounting and backward-compatible alternative.

If the key is absent or invalid, use `clockwise_90` for this installation. `--camera-config` continues to override only the SDK calibration directory and does not change orientation.

The P2 device page adds a `画面方向` combo box under capture parameters. Applying a changed orientation updates the shared settings and restarts the camera through the existing safe restart path.

## Image and Geometry Transformation

Replace the fixed counterclockwise-only helpers with orientation-aware helpers used by both `CaptureWindow` and `CaptureSession`.

For `clockwise_90`:

- Rotate RGB and display-depth pixels with OpenCV `ROTATE_90_CLOCKWISE`.
- Rotate spatial depth pixels identically.
- Transform every spatial point `(X, Y, Z)` to `(-Y, X, Z)`.

For `counterclockwise_90`:

- Keep the existing OpenCV `ROTATE_90_COUNTERCLOCKWISE` behavior.
- Keep the existing spatial transform `(X, Y, Z)` to `(Y, -X, Z)`.

No extra preview-only flip is allowed. Each frame is transformed exactly once before it is published, previewed, or saved.

## Ownership and Compatibility

- `default_camera.ini` owns the selected orientation.
- `ProcessingSettingsStore` preserves comments and unknown keys while loading and saving the new key.
- UI capture and CLI capture consume the same typed camera setting.
- Multiview and Elemental flip keys remain unchanged because they control renderer/output conventions, not physical camera mounting.
- SDK calibration files (`jp.xml`, `param.txt`, and `.cen`) remain untouched.

## Validation and Testing

- Unit-test clockwise and counterclockwise pixel mappings.
- Unit-test corresponding spatial XYZ mappings.
- Assert RGB and depth remain pixel-aligned and do not alias SDK storage.
- Test missing/invalid configuration fallback and persistence.
- Assert main-window and CLI capture use the shared orientation-aware helpers exactly once.
- Run camera orientation, processing settings, save settings, printing, and pipeline regression suites.
- Stop any running `mergeholo.exe`, rebuild the official `00-bin/mergeholo.exe`, and run the CLI smoke check. Do not create a verification executable.

## Out of Scope

- Arbitrary-angle rotation.
- Horizontal mirroring.
- Changes to multiview camera orbit or Elemental output direction.
- Editing SDK calibration data.

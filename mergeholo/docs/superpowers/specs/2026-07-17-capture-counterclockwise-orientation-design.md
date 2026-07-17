# Capture Counterclockwise Orientation Design

## Goal

Correct the camera orientation at the capture boundary so RGB, depth, preview, saved pipeline input, mesh, multiview, elemental, and printing all use frames rotated 90 degrees counterclockwise.

## Scope

- Rotate `img2d`, `img3d`, and `depthMap` with OpenCV `cv::ROTATE_90_COUNTERCLOCKWISE`.
- Apply the same operation to the main-window live capture path and the standalone `--capture` path.
- Keep RGB and three-channel depth pixels in the same transformed row/column order.
- Preserve the OpenCV matrix type and channel count; only rows, columns, and pixel positions change.
- Empty matrices remain empty.

## Architecture

Add a focused camera utility with this interface:

```cpp
cv::Mat rotateCaptureCounterClockwise90(const cv::Mat& source);
```

The function returns a newly owned matrix. It does not mutate or alias the camera SDK buffer.

`CaptureWindow::pollCameraFrame()` applies the helper immediately after dequeuing the newest `HoloOutData`:

- `frame.img2d` becomes `latestRgb_` after rotation;
- `frame.img3d` becomes `latestDepthForPipeline_` after rotation;
- `frame.depthMap`, or `frame.img3d` when the display map is absent, becomes `latestDepthDisplay_` after rotation.

The existing preview, freeze, pipeline-input write, mesh, multiview, elemental, and print paths remain unchanged because they already consume these `latest*`/`frozen*` values.

`runCaptureSession()` applies the same helper to `img2d` and `img3d` before preview, movement detection, and file writes. This keeps command-line capture archives consistent with the UI pipeline.

## Data Mapping

For an input matrix with `R` rows and `C` columns, the output has `C` rows and `R` columns. Every source pixel `(row, column)` maps to:

```text
output row    = C - 1 - column
output column = row
```

The same mapping is applied independently to RGB, three-channel floating-point depth, and the display depth map.

## Behavior

- The main-window RGB and depth previews appear upright rather than sideways.
- Clicking Capture freezes the already rotated frame; no second rotation occurs.
- `output/input/0.jpg` and `output/input/0.tiff` contain the rotated data.
- Depth-to-point-cloud, mesh, multiview, elemental, and printing receive the rotated pair without code changes in those stages.
- `--capture` preview and saved 2D/3D files use the same orientation.
- Width and height are intentionally swapped after the rotation.

## Error Handling

- An empty source returns an empty matrix.
- OpenCV allocation or rotation failures propagate through the existing capture error boundaries; no partially transformed RGB/depth pair is published.
- Existing camera, file-write, pipeline, and hardware error handling remains unchanged.

## Verification

- Add a focused console test using a non-square matrix with unique values to prove the exact counterclockwise mapping.
- Test `CV_8UC3` and `CV_32FC3` matrices to prove type/channel preservation and matching RGB/depth mapping.
- Test empty input.
- Add integration assertions that both `CaptureWindow.cpp` and `CaptureSession.cpp` call the shared helper for RGB and three-channel depth.
- Run the orientation test, existing save-settings test, persistence test, printing test, elemental test, and a full Release build.
- Launch the main window and visually verify that both previews are upright and remain aligned.

## Non-goals

- Do not rotate mesh, multiview, or elemental output a second time.
- Do not change camera SDK parsing, calibration values, depth coordinate components, print timing, second-screen presentation, motion control, exposure, cancellation, or homing logic.
- Do not add a user-selectable rotation setting in this change.
- Do not revert or clean the existing UI-style and 9030 worktree changes.

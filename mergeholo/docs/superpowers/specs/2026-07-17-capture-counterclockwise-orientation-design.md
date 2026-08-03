# Capture Counterclockwise Orientation Design

## Goal

Correct the camera orientation at the capture boundary so RGB, depth, preview, saved pipeline input, mesh, multiview, elemental, and printing all use frames rotated 90 degrees counterclockwise.

## Scope

- Rotate `img2d`, `img3d`, and `depthMap` pixel positions with OpenCV `cv::ROTATE_90_COUNTERCLOCKWISE`.
- Rotate each `img3d` spatial point from `(X,Y,Z)` to `(Y,-X,Z)` so generated point-cloud geometry follows the image orientation.
- Apply the same operation to the main-window live capture path and the standalone `--capture` path.
- Keep RGB and three-channel depth pixels in the same transformed row/column order.
- Preserve the OpenCV matrix type and channel count. RGB and display-depth values stay unchanged; `img3d` X/Y components follow the spatial rotation defined below.
- Empty matrices remain empty.

## Architecture

Add a focused camera utility with these interfaces:

```cpp
cv::Mat rotateCaptureCounterClockwise90(const cv::Mat& source);
cv::Mat rotateCaptureDepthCounterClockwise90(const cv::Mat& source);
```

Both functions return a newly owned matrix and do not mutate or alias the camera SDK buffer. The generic function rotates only pixel positions. The spatial-depth function accepts `CV_32FC3`, performs the same pixel rotation, and then maps every point `(X,Y,Z)` to `(Y,-X,Z)`.

`CaptureWindow::pollCameraFrame()` applies the helper immediately after dequeuing the newest `HoloOutData`:

- `frame.img2d` becomes `latestRgb_` after rotation;
- `frame.img3d` becomes `latestDepthForPipeline_` after pixel and XYZ rotation;
- `frame.depthMap`, or `frame.img3d` when the display map is absent, becomes `latestDepthDisplay_` after rotation.

The existing preview, freeze, pipeline-input write, mesh, elemental, and print paths consume these `latest*`/`frozen*` values. Multiview additionally uses the rotated camera basis and an absolute camera orbit so its row/column directions stay aligned with the rotated geometry.

`runCaptureSession()` applies the same helper to `img2d` and `img3d` before preview, movement detection, and file writes. This keeps command-line capture archives consistent with the UI pipeline.

## Data Mapping

For an input matrix with `R` rows and `C` columns, the output has `C` rows and `R` columns. Every source pixel `(row, column)` maps to:

```text
output row    = C - 1 - column
output column = row
```

The same pixel mapping is applied independently to RGB, three-channel floating-point depth, and the display depth map. For `img3d`, the coordinate transform is:

```text
X' =  Y
Y' = -X
Z' =  Z
```

The point-cloud loader maps SDK depth values to PCL as `(X,-Y,-Z)`. After the transform it emits `(Y,X,-Z)`, which is the 90-degree counterclockwise rotation `(-oldY,oldX,oldZ)` of the original PCL XY geometry.

## Behavior

- The main-window RGB and depth previews appear upright rather than sideways.
- Clicking Capture freezes the already rotated frame; no second rotation occurs.
- `output/input/0.jpg` and `output/input/0.tiff` contain the rotated data.
- Depth-to-point-cloud, mesh, elemental, and printing receive both the rotated pixel layout and rotated spatial geometry directly. Multiview uses `eye=+Z`, `up=+Y`, a left-to-right yaw orbit, and a top-to-bottom pitch orbit around the actual model center.
- Multiview uses symmetric sample-center poses across the full configured horizontal angle and half that range vertically, so the single-depth 2.5D face surface stays on its visible side without biasing either edge.
- Atlas and batch rendering restore the caller's scene, view matrix, and post-draw callback on success or failure; atlas readback also restores the previous OpenGL pack state if its sink throws.
- `--capture` preview and saved 2D/3D files use the same orientation.
- Width and height are intentionally swapped after the rotation.

## Error Handling

- An empty source returns an empty matrix.
- OpenCV allocation or rotation failures propagate through the existing capture error boundaries; no partially transformed RGB/depth pair is published.
- Existing camera, file-write, pipeline, and hardware error handling remains unchanged.

## Verification

- Add a focused console test using a non-square matrix with unique values to prove the exact counterclockwise mapping.
- Test `CV_8UC3` and `CV_32FC3` matrices to prove type/channel preservation, matching RGB/depth pixel mapping, and exact `(X,Y,Z)` transformation.
- Test empty input.
- Add integration assertions that both `CaptureWindow.cpp` and `CaptureSession.cpp` call the shared helper for RGB and three-channel depth.
- Run the orientation test, existing save-settings test, persistence test, printing test, elemental test, and a full Release build.
- Launch the main window and visually verify that both previews are upright and remain aligned.

## Non-goals

- Do not rotate mesh, multiview, or elemental output a second time.
- Do not change camera SDK parsing, calibration values, print timing, second-screen presentation, motion control, exposure, cancellation, or homing logic.
- Do not add a user-selectable rotation setting in this change.
- Do not revert or clean the existing UI-style and 9030 worktree changes.

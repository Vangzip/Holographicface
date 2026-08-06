# Input JPG Brightness Design

## Goal

Provide a UI-controlled brightness increment that is applied to the RGB JPG
before RGB-plus-depth point-cloud reconstruction. The resulting PLY colours,
mesh texture, multiview images, and Elemental output therefore use the same
brighter source colours.

## User control and persistence

- Add `输入 JPG 增亮（点云颜色）` to the existing **输出规格** group.
- It is an integer spin box with range `0..255`, default `0`.
- Persist the value in `default_pipeline.ini` as
  `input_jpg_brightness_increment`.
- The setting remains visible for every input mode, but only affects a depth
  stage that consumes a matching RGB JPG and TIFF pair.

## Processing behaviour

- For an increment of `0`, pass the original JPG path to the depth stage and
  perform no extra image work.
- For a positive increment, read the RGB JPG, derive the background colour
  from its four corners, and retain pixels within `8` RGB levels of that
  corner-colour median.
- Increase all other RGB channels by the configured amount, clamping at 255.
- Write the transformed image to a unique per-run temporary directory under
  the pipeline output root and pass that temporary JPG to the point-cloud
  converter.
- Delete the temporary directory on both success and failure. Never modify
  the original input JPG or TIFF.

## Data flow

`ProcessingSettingsDialog` → `PipelineUiSettings` →
`input_jpg_brightness_increment` in pipeline config → `HoloConfig` →
`DepthStage` temporary JPG → `depthToPointCloudColor` / `depthToPlyColor`.

## Validation

- Unit tests cover persistence, UI binding, background preservation,
  non-background saturation, and a depth-stage call using the transformed JPG.
- Existing zero-increment processing continues to use the original JPG path.
- A manual RGB-plus-depth run confirms the original JPG byte content is
  unchanged and the resulting point-cloud colour values are brighter.

# MergeHolo Unified Settings Dialog Design

**Date:** 2026-07-21

## Goal

Replace the separate main-window settings entry points with one native Qt `处理设置` dialog. Organize settings by operator priority, keep the existing Windows/Qt visual style, preserve the existing standalone 9030 print dialog, and keep current configuration files compatible.

The main window retains one `设置` button. Opening it shows a modal dialog with these navigation entries:

| Navigation | Badge | Purpose |
| --- | --- | --- |
| 常用 | P0 | Per-run input, output, dimensions, and result selection |
| 成像 | P1 | Subject framing, orientation, and image quality |
| 高级 | P2 | Point-cloud, mesh, and multiview performance parameters |
| 设备 | P2 | Camera connection and capture parameters |
| 打印 | P3 | Opens the existing standalone 9030 print dialog |

`打印` is a first-class P3 navigation entry, but it is not embedded into the settings page. Activating it launches the existing `Print9030Dialog` with its current layout and workflow.

## Selected Approach

Use one `ProcessingSettingsDialog` with a fixed left navigation list and a right-side stacked page area. Each page edits a shared typed settings draft. Nothing is written until the operator presses `应用`.

This approach was selected over separate dialogs for every category because it gives the main window one predictable entry point and makes priority visible. It was selected over a single scrolling form because the complete parameter set is too large and contains several settings that should not be exposed to ordinary operators.

The print workflow is intentionally the exception: its source selection, progress, start, cancel, and close states form an operation rather than ordinary application preferences. It therefore remains an independent dialog.

## Shared Window Layout

- Native Qt Widgets controls and the existing Windows gray-white visual style.
- Modal title: `处理设置`.
- Fixed left navigation; selected item uses the existing pale blue-gray selection color.
- Right-side page uses aligned group boxes and native spin boxes, combo boxes, check boxes, and path selectors.
- Bottom-left action: `恢复默认值`, scoped to the selected page.
- Bottom-right actions: `取消` and `应用`.
- `取消` discards the draft and leaves active configuration unchanged.
- `应用` validates the whole draft, persists supported values, and updates the next processing run.
- Camera parameters that require reinitialization are marked in the page and applied only after safe camera restart.
- Settings that cannot be modified while capture, processing, or printing is active are disabled.

## P0 常用

### 输入与输出

- `输入来源`: camera, RGB/depth directory, mesh directory, or multiview directory.
- `输入目录`: enabled only for an external input source; reuse the current input validation contracts.
- `输出目录`: the root directory for generated results.

### 输出规格

- `视角范围` maps to `multiview_angle`.
- `每度采样数` maps to `multiview_per`.
- `单视图分辨率` maps to `multiview_resolution`; the current implementation is square, so the UI shows `N × N` and stores one value.
- `Elemental 行列` maps to `target_rows` and `target_cols`.

Initial control ranges are `1–360` degrees, `1–10` samples per degree, `16–4096` pixels for one view, and `1–4096` for each Elemental target dimension. Cross-field validation and the memory estimate may reject a combination even when each individual value is in range.

The page shows a live read-only summary:

```text
视点行数 = 视点列数 = multiview_angle × multiview_per
Elemental 图像数 = target_rows × target_cols
```

Derived values such as `view_rows` and `view_cols` are never independently editable.

### 保存内容

- `网格模型` maps to `save_mesh_result`.
- `多视图图像` maps to `save_multiview_result`.
- `Elemental 图像` maps to `save_elemental_result`.

This absorbs the current input and save settings into the unified dialog without changing their underlying validation or result-persistence behavior.

## P1 成像

The UI uses operator language instead of exposing OSG vectors.

### 人物构图

- `人物大小` maps inversely to `multiview_camera_distance_scale` using `distance_scale = 4.0 / displayed_size`. This preserves the accepted default display value `2.0` for the current distance scale `2.0`, while making a larger displayed value produce a larger subject. The displayed range is `0.5–5.0`.
- `左右位置`, `上下位置`, and `远近位置` map directly to `multiview_camera_center_offset_x/y/z`, respectively. Their initial displayed range is `-1.0–1.0`.
- `居中` resets the three offsets without changing other imaging values.

### 姿态校正

- `水平旋转` maps to `multiview_initial_rotate_z_deg`.
- `俯仰旋转` maps to `multiview_initial_rotate_x_deg`.
- `重置姿态` restores both rotations to zero.

Both rotation controls use `-180.0–180.0` degrees and follow the corrected capture orientation already established in the pipeline. The UI must not duplicate rotation or apply it to imported external images.

### 图像质量与方向

- `JPEG 质量` maps to `jpg_quality`.
- `垂直翻转采集图像` maps to `multiview_capture_flip_vertical`.
- `Elemental 输出方向` presents four named direction presets: `标准方向` = `(false, false)`, `源图垂直翻转` = `(true, false)`, `视点行翻转` = `(false, true)`, and `双向翻转` = `(true, true)` for `(elemental_flip_source_y, elemental_flip_view_rows)`.

## P2 高级

The page begins with a restrained warning that these settings affect reconstruction quality and performance.

### 点云生成

The primary page exposes:

- Calibration preset.
- `focus`.
- `disp`.
- `step`.
- Enable/disable outlier filtering.

`详细参数` opens a subordinate dialog for `label`, `fdis`, `greenRGB`, `meanK`, `stddevMulThresh`, `radiussearch`, and `minNeighborInRadius`.

### 网格重建

The primary page exposes:

- `reconstruct` as a named algorithm choice.
- `searchradius`.
- `kSearch`.
- `leafsize`.

`详细参数` opens a subordinate dialog for `mu`, `maximumNearestNeighbors`, `maximumSurfaceAngle`, `minimumAngle`, `maximumAngle`, `holesize`, `mlsSearchRadius`, `normalsFitIter1`, `normalsFitIter2`, `neighbor_num`, `nearest_distance`, and mesh texturing `focus`.

Algorithm-specific fields are enabled only when used by the selected reconstruction algorithm.

### 多视图与性能

- `纹理图集大小` maps to `multiview_atlas_size`, with `自动（推荐）` represented by zero.
- `处理线程数` maps to `elemental_writer_threads`, with `自动` represented by zero.
- `启用硬件自适应` is a UI-level switch. When checked, it writes zero to both atlas size and thread count and disables their manual controls. When unchecked, the two controls accept explicit values.

File paths such as `depth_config` and `mesh_config` are implementation details. The UI edits typed values and the settings layer persists them to the compatible files.

## P2 设备

The device page contains camera settings only. Printing and motion-axis settings do not appear here.

### 相机连接

- `配置方案`: initially `084C 默认方案`.
- `配置目录`: maps to `camera_config_dir`.
- Read-only directory and connection status.
- `测试连接` is available only while idle. It validates the draft preset, releases the current camera, initializes with the draft settings, waits up to five seconds for one valid frame, then releases the test instance and restores the previously applied camera settings. Failure leaves the previously applied settings active.
- `重新初始化` releases and safely initializes the camera using the current applied settings.

Changing the camera configuration directory requires reinitialization. The selected directory must contain the required SDK files before it can be applied.

### 采集参数

- `曝光模式` maps to `iHoloExposeMode`; current value is `1`. The first version labels this supported value `手动`; no unverified SDK mode is offered.
- `曝光值` maps to `iHoloExposeVal`; current value is `15000`.
- `帧率` maps to `dHoloFrameRate`; current value is `6.0`.

These values are currently hard-coded in both the UI capture path and command-line capture path. The unified settings model becomes their shared source.

### 相机信息

The main page shows these values read-only:

- Camera interface/driver identifier: current `571` (`strCamSeri`).
- Camera type: current `Indigo` (`strCamType`).
- Camera ID: current `0` (`iHoloId`).
- GPU ID: current `0` (`iGpuId`).

`工程师设置` allows authorized editing of those four fields plus the missed-frame threshold (`iHoloMissThreshold`, current `100`). The first version does not add an authentication system; the engineering separation is a UI safety boundary and warning, not a security boundary.

Serial temperature-reading settings remain hidden because the feature is disabled (`bIsReadTeamptureBySerial = false`).

### SDK Calibration Files

The `084C` directory contains `jp.xml`, `param.txt`, `0_39.90.png.cen`, supporting image data, and SDK batch files.

- `param.txt` and `.cen` calibration data must never be edited through this dialog.
- `jp.xml` contains coupled SDK parsing and denoising parameters. It is switched as part of a complete camera configuration preset, not edited field by field.
- A preset is accepted only when all required files are present.

This prevents an operator from combining XML values with incompatible calibration data.

## P3 打印

Selecting `打印` opens the existing `Print9030Dialog`. The current dialog layout and operational behavior remain intact:

- Image source: in-memory Elemental result or manual folder.
- `打印参数` tab with the existing main print fields.
- `运动轴` tab with the existing X/Y/Z/W axis table.
- Print progress bar and status.
- `打印`, `取消`, and `关闭` actions.
- Existing `print_9030.ini` load/save behavior.
- Existing print runner, cancellation, image-source validation, and motion controller behavior.

No print or axis field is duplicated into `ProcessingSettingsDialog`. The P3 entry is navigation to the existing operation dialog, not another configuration implementation.

## Configuration Ownership

A typed application settings model is the single in-memory source of truth. Adapters translate between that model and existing files:

- `default_camera.ini`: camera configuration directory and new ordinary camera settings.
- Pipeline configuration/template: P0, P1, and multiview performance values.
- `depth_to_pointcloud_config.cfg`: point-cloud values.
- `mesh_config.cfg`: mesh values.
- `print_9030.ini`: remains owned exclusively by `Print9030Dialog`.

The runtime-generated `output/holo_config.ui.ini` remains generated output and is not directly edited by the operator.

Existing example, merge, and microtest files remain developer artifacts. Applying UI settings must not silently rewrite them.

Unknown keys and comments in user-editable configuration files should be preserved where practical. Persistence must use atomic replacement or `QSaveFile` so a failed write does not corrupt a working configuration.

## Data Flow

```text
Main window 设置
  -> load typed settings from compatible files
  -> edit a draft in ProcessingSettingsDialog
  -> validate page fields and cross-field relationships
  -> 应用
      -> atomically persist owned configuration values
      -> update CaptureWindow settings
      -> if camera values changed, mark restart required

P3 打印
  -> open existing Print9030Dialog
  -> existing print_9030.ini and PrintJobRunner flow
```

The main window continues generating the per-run pipeline configuration from the applied typed settings. CLI behavior remains compatible with explicitly supplied configuration files and flags.

## Validation and Errors

- Paths must exist when required and must be writable for output.
- External input validation reuses the current RGB/depth, mesh, and multiview contracts.
- All numeric controls have explicit safe ranges; invalid text is not accepted.
- `multiview_angle`, `multiview_per`, and resolution must produce a supported nonzero render plan.
- Estimated output sizes that are unusually large require confirmation.
- Camera presets must include the required SDK calibration files.
- Camera reinitialization is blocked while capture or processing is active.
- Print configuration and axis errors continue to be handled inside the existing print dialog.
- If any persistence step fails, the active settings remain unchanged and the dialog reports the specific file and error.

## Compatibility and Migration

- Existing configuration keys retain their current meanings.
- Existing input, save, and print behavior remains available through the unified entry point.
- Existing command-line configuration paths remain supported.
- Missing newly introduced camera keys fall back to the current hard-coded values, so old installations continue to work.
- The UI does not edit SDK calibration internals or developer test configurations.

## Verification

### Settings model tests

- Load current defaults from all owned files.
- Round-trip each exposed field without losing unrelated values.
- Preserve fallback behavior when new camera keys are absent.
- Reject invalid ranges and incompatible parameter combinations.
- Verify atomic-write failure leaves original files intact.

### Dialog tests

- Navigation order and P0/P1/P2/P2/P3 badges.
- Page-specific default restoration.
- Cancel discards changes; Apply commits a valid draft.
- Derived view and Elemental summaries update correctly.
- Camera fields correctly mark reinitialization required.
- Busy states disable device modification.
- P3 opens the existing `Print9030Dialog` and does not duplicate its controls.

### Integration and regression tests

- Camera and external-input processing generate the expected runtime pipeline configuration.
- Existing save-result behavior remains unchanged.
- Existing `Print9030Dialog` tests continue to pass unchanged unless only its launch location changes.
- Existing CLI configurations still load.
- Release build and UI smoke test verify native style, keyboard navigation, readable labels, and modal ownership.

## Out of Scope

- Editing `param.txt`, `.cen`, or individual `jp.xml` values.
- Replacing the existing print workflow or redesigning its dialog.
- Adding user authentication or role management for engineer settings.
- Changing pipeline algorithms or camera SDK behavior.
- Rewriting example, compatibility, or microtest configurations from the UI.

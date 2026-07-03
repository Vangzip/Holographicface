# Holo integrated project

`Holo` is the standalone full-pipeline program. It no longer calls `point_cloud.exe`, `testmulitview.exe`, or `Generateimages.exe`.

The required source/header files from the old projects are copied under `vendor/` and compiled into `Holo` directly. The project still depends on the required third-party libraries: OpenCV, PCL, VTK, OSG, and osgEarth.

## Pipeline

1. `depthImage`: depth TIFF + JPG to colored PLY point cloud.
2. `ConverPointCloud`: PLY point cloud to mesh PLY.
3. `OdmTexturing`: mesh PLY + JPG texture to textured OBJ.
4. `modelMoveHandler`: OBJ to multiview JPG images.
5. `Holo elemental`: multiview images to target elemental images.

## Target size

To generate `150 * 150` output images, each `270 * 270` pixels, use:

```ini
output_root=output\full_150x150_res270
multiview_out_dir=multiview
elemental_out_dir=elemental
multiview_angle=30
multiview_per=9
multiview_resolution=150
target_rows=150
target_cols=150
```

Meaning:

- `multiview_angle * multiview_per = 270`, so multiview creates `270 * 270 = 72900` view images.
- `multiview_resolution=150`, so each view image is `150 * 150`.
- `target_rows * target_cols = 150 * 150`, so final output count is `22500`.
- Each final output image is `270 * 270` pixels, derived from `multiview_angle * multiview_per`.

## Usage

Open `Holo.sln` to load only the Holo project. Holo is x64-only because the current OpenCV, PCL, OSG, and osgEarth dependencies are x64.

Use the example config directly, or copy it before editing:

```bat
copy holo_config.example.ini holo_config.ini
```

Dry-run the pipeline:

```bat
target\Holo.exe --config holo_config.ini --dry-run
```

Run only elemental image generation:

```bat
target\Holo.exe --config holo_config.ini --stage elemental
```

Run the full pipeline:

```bat
target\Holo.exe --config holo_config.ini --stage all
```

Run the 0-sample micro functional test:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File run_microtest.ps1
```

For Chinese usage notes, see `使用说明.md`.

## Notes

- `model_type=obj` is the integrated multiview path currently supported by Holo.
- The copied vendor sources are Holo-owned snapshots. Changes here do not modify the old projects.
- `depth_to_pointcloud_config.cfg` and `mesh_config.cfg` are now local Holo config files.
- The Holo post-build step copies required third-party runtime DLLs and OSG plugins into `target`.

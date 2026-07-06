# MergeHolo

MergeHolo combines the two existing local projects without modifying them:

- `Holo`: full pipeline from depth TIFF + RGB JPG to point cloud, mesh, OBJ, multiview images, and elemental images.
- `holocamera`: Qt/OpenCV light-field camera capture using the `JpLF-v3.1` SDK.

The old `Holo` and `holocamera` directories are treated as read-only source snapshots. New source, configs, scripts, and outputs live under `mergeholo`.

## Layout

```text
mergeholo/
  apps/                 unified CLI entry
  camera/               light-field capture wrapper and copied camera sources
  pipeline/             Holo pipeline wrapper and capture import bridge
  vendor/               copied Holo-owned vendor sources
  config/               MergeHolo pipeline configs
  Pri/                  qmake dependency settings
  scripts/              build and run helpers
  samples/face_roate/   local sample input copied from Holo, ignored by git
  runs/                 generated capture and pipeline output, ignored by git
```

## Build

```powershell
cd C:\wzp\Holographicface\mergeholo
.\scripts\build.ps1 -Config release -Clean
```

The build uses Qt 5.15 MSVC2019 x64 by default:

```text
C:\wzp\QT\5.15.0\msvc2019_64
```

Override paths if needed:

```powershell
.\scripts\build.ps1 -QtRoot "C:\Qt\5.15.0\msvc2019_64" -VsDevCmd "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
```

## Run Modes

Capture only:

```powershell
.\00-bin\mergeholo.exe --capture --save-dir .\runs\latest\capture --max-frames 10
```

Import captured frames for the Holo pipeline:

```powershell
.\00-bin\mergeholo.exe --import-capture --capture-dir .\runs\latest\capture --pipeline-input .\runs\latest\pipeline_input
```

Capture, import, then run the Holo pipeline:

```powershell
.\00-bin\mergeholo.exe --capture-and-run --max-frames 1 --stage all
```

Run only the Holo pipeline:

```powershell
.\00-bin\mergeholo.exe --pipeline --config .\config\holo_config.merge.ini --stage all
```

Run the copied micro sample:

```powershell
.\scripts\run_microtest.ps1 -Stage all
```

Dry-run the sample without generating output:

```powershell
.\scripts\run_microtest.ps1 -Stage all -DryRun
```

## Fusion Point

The camera side saves:

```text
runs/latest/capture/2d/<timestamp>.jpg
runs/latest/capture/3d/<timestamp>_3D.tiff
```

`pipeline/CaptureImport.cpp` converts that layout into the Holo input convention:

```text
runs/latest/pipeline_input/<timestamp>.jpg
runs/latest/pipeline_input/<timestamp>.tiff
```

Then `pipeline/HoloPipeline.cpp` runs the original Holo stages against `config/holo_config.merge.ini`.

## Runtime Resources

`holoLib` is close to 2GB, so it is not duplicated in source control. The project supports either:

- `mergeholo/runtime/holoLib`, or
- read-only fallback to `..\holocamera\HoloTest\holoLib`.

`scripts/build.ps1` deploys the required DLLs and camera config into `mergeholo/00-bin` after a successful build.

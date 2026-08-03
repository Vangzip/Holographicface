# MergeHolo Project Code Index

This is the maintained-source navigation map for MergeHolo. It records project
ownership and direct source-level relationships; it is not generated API
reference documentation.

## How To Read And Maintain This Index

Start with the runtime graph for an end-to-end request, then open the matching
module section. Each type card has this form:

```text
Source: defining file(s)
Role: one responsibility in the system
Depends on: direct project dependencies; external boundaries in brackets
Used by: project callers or owning flow
Boundary: persistent state, input, output, or lifecycle owned by the type
```

Scope includes maintained source under `apps`, `camera`, `pipeline`,
`printing`, `settings`, `widgets`, `vendor/base`, `vendor/point_cloud`, and
`vendor/multiview`. Qt, OpenCV, PCL, OSG, CUDA, JpLF v4.1.1, and IMC60G are
external boundaries: their project-side adapters and configuration points are
indexed, their internal implementation is not.

Update this file when `mergeholo.pro`, `pipeline/PipelineModule.pri`, a public
project type, a module boundary, or an SDK adapter changes. Paired `.cpp`
files are covered by their corresponding header card unless a file implements a
coherent free-function module.

## Build Ownership And External Boundaries

`mergeholo.pro` is the top-level qmake manifest. It defines the `mergeholo`
Qt Widgets executable, owns application/camera/printing/settings/widgets and
maintained vendor source lists, and includes the shared build fragments.
`pipeline/PipelineModule.pri` owns the pipeline source and header lists.

| Build owner | Responsibility | Key boundary |
| --- | --- | --- |
| `Pri/common.pri` | C++ source encoding, MSVC v142 guard, qmake intermediate directories, output path | Deploys the executable to `00-bin/`; intermediates go to `FF-tmp/`. |
| `Pri/imc60g.pri` | IMC60G include, import library, runtime presence checks | `vendor/imc60g` SDK, x64 only. |
| `Pri/opencv.pri` | OpenCV headers and Release/Debug link selection | OpenCV 4.5.0, discovered through `OPENCV_ROOT`. |
| `Pri/holo_pipeline.pri` | PCL, Boost, OSG, osgEarth, OpenNI2, VTK, Qhull linkage | MSVC v142 x64 paths and `/arch:AVX`. |
| `Pri/cuda.pri` | Optional CUDA include/link setup | Builds with `no_cuda` if the toolkit is absent. |
| `Pri/ffmpeg.pri` | Retained FFmpeg include/link fragment | Local FFmpeg installation; not currently included by `mergeholo.pro`. |
| `Pri/eigen5.pri` | Retained Eigen include fragment | Header-only Eigen installation; not currently included by `mergeholo.pro`. |
| `mergeholo.pro` | Qt modules, JpLF SDK lookup, Win32 D3D linkage | Requires `JP_LF_V4_ROOT` or the sibling `holocamera` SDK path. |

The compilation manifest is authoritative for active production source. Some
retained headers outside its explicit lists are documented below because they
are project-owned compatibility/support code.

## Runtime And Data Flow

The default interactive flow is:

```text
CaptureWindow
  -> LightFieldCapture live RGB/depth frames
  -> frozen capture confirmation
  -> CaptureImport RGB + TIFF input pair
  -> HoloPipeline depth -> mesh -> multiview -> elemental
  -> optional persisted result and printing selection
```

The CLI entry also supports capture-only, import-only, and pipeline-only modes.
The detailed runtime graph is in the maintained vendor section after all module
arrows are established from source.

## Module Dependency Map

The detailed module graph is maintained with the runtime graph near the end of
this index. Its arrows point from caller to direct project dependency; external
libraries are grouped as boundary nodes.

## Directory And File Roles

| Path | Identity | Notes |
| --- | --- | --- |
| `apps/` | executable entry | Command dispatch and application bootstrap. |
| `camera/` | capture integration | Light-field capture, frame orientation, session control, and retained camera support headers. |
| `config/` | versioned runtime configuration | Default processing, camera, mesh, and printing profiles. |
| `docs/` | project knowledge | Operator guides, design/plan records, and this index. |
| `pipeline/` | processing domain | Capture input, five processing stages, in-memory results, persistence, and timing. |
| `Pri/` | qmake build fragments | Compiler settings, output locations, and external libraries. |
| `printing/` | 9030 print domain | Job control, screen presentation, exposure timing, motion, and SDK adapters. |
| `scripts/` | build and test entry points | PowerShell wrappers around qmake, tests, and hardware acceptance. |
| `settings/` | persisted UI configuration | Key/value INI access and unified processing settings. |
| `ui/` | Qt Designer forms | Layouts owned by the widgets module. |
| `vendor/base/` | maintained base utilities | File and logging helpers compiled into the application. |
| `vendor/multiview/` | maintained rendering module | OSG render planning, atlas creation, camera motion, and memory sinks. |
| `vendor/point_cloud/` | maintained geometry module | Depth-to-point-cloud, mesh reconstruction, and texturing adapters. |
| `vendor/imc60g/` | binary SDK dependency | Header, import library, and runtime DLL; no project logic belongs here. |
| `00-bin/` | deployed runtime output | Ignored build/deployment output, not source. |
| `FF-tmp/` | qmake intermediate output | Ignored `moc`, `uic`, object, and test build artifacts. |
| `build/`, `debug/`, `release/` | local build products | Ignored build state. |
| `output/`, `runs/` | runtime output | Ignored processing, capture, log, and acceptance data. |
| `samples/` | optional input data | Ignored demo or local validation inputs. |
| `.qmake.stash`, `Makefile*`, `*.obj` | generated local state | Regenerated by qmake/nmake and ignored. |

## Application, UI, Settings, And Camera

The application, UI, settings, and camera type index is maintained in this
section.

## Pipeline

The pipeline type index is maintained in this section.

## Printing

The printing type index is maintained in this section.

## Maintained Vendor Modules

The maintained vendor type index and Mermaid graphs are maintained in this
section.

## External SDK And Library Boundaries

The detailed external boundary index is maintained in this section.

## Configuration, Scripts, And Tests

The configuration, scripts, and test ownership index is maintained in this
section.

## Coverage Checklist

The final checklist records all documented source roots, explicit paired-source
exceptions, and verification commands.

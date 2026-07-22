# Mesh Reconstruction I/O Decoupling Design

**Date:** 2026-07-22

## Goal

Make Poisson reconstruction a working selectable reconstruction method while separating point-cloud input and mesh output from both reconstruction algorithms. The default camera pipeline continues to pass point clouds and meshes entirely in memory.

The existing configuration meaning remains unchanged:

- `reconstruct=1`: Poisson reconstruction.
- `reconstruct=2`: Greedy Projection Triangulation and the default algorithm.

## Current Behavior

The settings UI and `mesh_config.cfg` already expose both reconstruction methods. The file-based mesh path can run either method, but the newer in-memory path explicitly rejects every method except Greedy Projection Triangulation.

The two implementations also have different ownership boundaries:

- Greedy reconstruction has an in-memory entry point, but it still receives a logical source path and a `writeMeshFile` flag.
- Poisson reconstruction loads its own PLY input and its crop-hull post-processing always saves a PLY file.

As a result, algorithms are coupled to how data is read, named, and written, and selecting Poisson in the live camera pipeline fails.

## Architecture

The mesh stage is divided into three responsibilities:

```text
file or camera input
        |
        v
point-cloud input adapter
        |
        v
pcl::PointCloud<pcl::PointXYZRGB>
        |
        v
reconstruction dispatcher
        +-- Poisson reconstruction
        +-- Greedy Projection Triangulation
        |
        v
pcl::PolygonMesh
        |
        +-- pass directly to the model stage (default)
        +-- mesh output adapter saves PLY when requested
```

Input and output adapters own all filesystem behavior. Reconstruction functions receive an in-memory point cloud plus reconstruction parameters and return an in-memory polygon mesh. They do not receive file paths, output directories, or write flags.

This separation is introduced inside the existing `ConverPointCloud` boundary rather than adding a new class hierarchy. It provides the required module boundary with fewer changes to the legacy point-cloud library. A strategy-class hierarchy can be introduced later if additional reconstruction methods make dispatch or configuration difficult to maintain.

## Reconstruction Contract

The common reconstruction dispatcher will:

1. Validate that the input cloud exists and is non-empty.
2. Select the implementation using the parsed `reconstruct` value.
3. Pass the same immutable input-cloud contract to either algorithm.
4. Return failure when the selected method is unknown or produces no vertices or polygons.
5. Return a complete `pcl::PolygonMesh` without performing file I/O.

Algorithm-specific preprocessing and post-processing remain inside each reconstruction module because they are part of producing that algorithm's usable mesh:

- Poisson owns voxel filtering, normal estimation, Poisson reconstruction, convex-hull cropping, and removal of unused vertices.
- Greedy owns voxel filtering, normal estimation, Greedy Projection Triangulation, and hole filling.

Neither implementation owns mesh path construction or serialization.

## Poisson In-Memory Adaptation

The existing Poisson implementation under `vendor/point_cloud` is the source of truth. Equivalent copies under the parent `Holographicface` tree are older file-oriented variants and are used only as reference.

The existing Poisson settings are preserved initially:

- confidence enabled
- degree `2`
- depth `8`
- iso divide `8`
- manifold enabled
- output polygons disabled
- samples per node `3`
- scale `1.1`
- solver divide `8`
- point weight `4.0`

The old file-based function is split so that loading a PLY is only an adapter step. The Poisson core receives the already available `PointXYZRGB` cloud and writes its reconstructed mesh to an output reference.

The crop-hull helper is changed from "crop and always save" to "crop into an output mesh." Conditional saving occurs only after reconstruction returns to the caller. This also removes the current heap-only helper lifetime from the normal call path and makes failures from cropping or saving observable by the caller.

## Input and Output Paths

Two public workflows remain supported:

### Default camera pipeline

The depth stage produces an in-memory point cloud. The mesh stage dispatches the selected algorithm and stores the returned mesh in `MeshMemoryResult`. The model stage consumes that mesh directly. No point-cloud or mesh file is required between these stages.

When result persistence is enabled, the same already reconstructed mesh is passed to the output adapter after reconstruction; the algorithm is not run a second time.

### Standalone file workflow

The existing file API loads PLY into the same in-memory point-cloud type, calls the same reconstruction dispatcher, and then saves the returned mesh. This preserves command-line and legacy file processing while removing the separate algorithm behavior.

Logical paths may still be used by the orchestration layer to derive final output names and texture-model metadata. They are not passed into the reconstruction implementations.

## Configuration and UI

No new reconstruction selector is required. The existing UI entries and config values remain:

- `泊松重建` -> `reconstruct=1`
- `贪婪三角化` -> `reconstruct=2`

Greedy remains the default to avoid changing existing installations merely by upgrading. Selecting Poisson must work in both the live in-memory pipeline and the standalone file workflow.

The first implementation preserves the currently hard-coded Poisson tuning values. Exposing Poisson tuning in the settings dialog is outside this change and can be designed separately after the algorithm is proven on representative captures.

## Error Handling

Failures are reported at the boundary that owns them:

- Input adapter: missing, unreadable, or empty point cloud.
- Reconstruction dispatcher: unsupported reconstruction value.
- Reconstruction module: invalid normals, empty reconstructed mesh, or algorithm exception/failure.
- Poisson post-processing: invalid or empty cropped mesh.
- Output adapter: failed mesh serialization.

The log identifies the selected method and whether failure happened during input, reconstruction, post-processing, or output. A Poisson failure may recommend switching to Greedy, but it must not silently change algorithms because that would make the saved configuration misleading.

## Testing and Validation

Tests will cover:

- Dispatcher selection for `reconstruct=1` and `reconstruct=2`.
- Rejection of unknown reconstruction values.
- Both algorithms accepting the same synthetic in-memory cloud contract.
- Poisson returning a non-empty in-memory mesh without requiring an input PLY or output PLY.
- File mode and memory mode producing equivalent mesh topology for the same point cloud and configuration.
- Conditional persistence saving the returned mesh without rerunning reconstruction.
- Existing Greedy memory behavior and model-stage handoff remaining compatible.
- Existing settings load/save behavior preserving both selector values.

Representative camera data should also be checked visually because Poisson reconstruction quality depends strongly on point density, normal orientation, and scale. The first acceptance target is a complete, non-empty, textureable mesh with no intermediate file dependency; tuning visual quality is a separate follow-up if the preserved legacy values are unsuitable for current captures.

## Build and Delivery

Before implementation, back up every source and header that will be changed. Preserve unrelated worktree changes.

For the final Windows build:

1. Terminate `mergeholo.exe` if it is running.
2. Delete `00-bin/mergeholo_verify.exe` if it exists.
3. Build only the official `00-bin/mergeholo.exe`.
4. Run focused reconstruction tests and the existing pipeline smoke checks.

## Out of Scope

- Changing the default algorithm from Greedy to Poisson.
- Automatically falling back from Poisson to Greedy.
- Exposing all Poisson tuning parameters in the UI.
- Replacing the legacy point-cloud library with a new framework.
- Changing depth generation, texture mapping, or multiview behavior.

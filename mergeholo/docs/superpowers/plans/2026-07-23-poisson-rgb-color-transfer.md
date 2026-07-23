# Poisson RGB Color Transfer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make every successful Poisson reconstruction return and persist a vertex-colored RGB mesh while preserving its geometry and polygon indices.

**Architecture:** Add one pure-memory color-transfer function beside the existing Poisson crop function. It converts the final cropped mesh vertices to `PointXYZRGB` by querying three nearest valid Poisson source points in a KD-Tree and inverse-distance weighting their colors; `ConverPointCloud` calls it after crop and treats any failure as reconstruction failure.

**Tech Stack:** C++17, PCL 1.12.1 (`PolygonMesh`, `KdTreeFLANN`, `PointXYZRGBNormal`, PLY IO), qmake, MSVC 2019, PowerShell.

## Global Constraints

- Reuse the legacy “reconstruction then color transfer” seam, but do not restore its invalid index-copy algorithm.
- Use only the valid-normal `PointXYZRGBNormal` cloud that actually enters Poisson reconstruction as the color source.
- Use at most three nearest neighbors with inverse Euclidean-distance weighting; exact coordinate matches copy the exact source color.
- Preserve vertex positions, vertex order, polygon count, and polygon indices.
- Keep Greedy reconstruction unchanged.
- Keep reconstruction and persistence decoupled; the new function performs no file IO.
- If color transfer fails, Poisson reconstruction fails and must not return a colorless mesh.
- Rebuild only `00-bin/mergeholo.exe`; kill a running `mergeholo.exe` first and ensure `00-bin/mergeholo_verify.exe` is absent.

---

## File Map

- Modify `vendor/point_cloud/include/poissonmesh.hpp`: declare the pure-memory `transferPoissonMeshColors` interface.
- Modify `vendor/point_cloud/src/poissonmesh.cpp`: implement deterministic three-neighbor RGB transfer.
- Modify `vendor/point_cloud/src/ConverPointCloud.cpp`: call color transfer after Poisson crop and fail clearly on error.
- Modify `vendor/point_cloud/tests/test_mesh_reconstruction.cpp`: add focused algorithm, failure, integration, and persisted-PLY tests.
- No build-system file changes are required because `poissonmesh.cpp` and the test source are already part of `mesh_reconstruction_tests.pro` and `mergeholo.pro`.

### Task 1: Pure-memory RGB transfer

**Files:**
- Modify: `vendor/point_cloud/include/poissonmesh.hpp`
- Modify: `vendor/point_cloud/src/poissonmesh.cpp`
- Test: `vendor/point_cloud/tests/test_mesh_reconstruction.cpp`

**Interfaces:**
- Consumes: a final geometry-only `pcl::PolygonMesh` and a non-empty `pcl::PointCloud<pcl::PointXYZRGBNormal>::ConstPtr`.
- Produces:

```cpp
bool transferPoissonMeshColors(
    const pcl::PolygonMesh& inputMesh,
    const pcl::PointCloud<pcl::PointXYZRGBNormal>::ConstPtr& sourceCloud,
    pcl::PolygonMesh& outputMesh);
```

- [ ] **Step 1: Declare the wished-for interface and write focused failing tests**

Add the declaration beside `cropPoissonMeshToPointCloudHull` in
`vendor/point_cloud/include/poissonmesh.hpp`.

Include the interface in the test:

```cpp
#include "ConverPointCloud.h"
#include "poissonmesh.hpp"
```

Add these helpers and tests to `test_mesh_reconstruction.cpp`:

```cpp
bool hasMeshField(const pcl::PolygonMesh& mesh, const std::string& name)
{
    for (const pcl::PCLPointField& field : mesh.cloud.fields) {
        if (field.name == name) {
            return true;
        }
    }
    return false;
}

pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr makeColorTransferSource()
{
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr source(
        new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    const struct {
        float x;
        float y;
        unsigned char r;
        unsigned char g;
        unsigned char b;
    } values[] = {
        {0.0f, 0.0f, 255, 0, 0},
        {2.0f, 0.0f, 0, 255, 0},
        {0.0f, 2.0f, 0, 0, 255}
    };
    for (const auto& value : values) {
        pcl::PointXYZRGBNormal point;
        point.x = value.x;
        point.y = value.y;
        point.z = 0.0f;
        point.r = value.r;
        point.g = value.g;
        point.b = value.b;
        point.normal_x = 0.0f;
        point.normal_y = 0.0f;
        point.normal_z = 1.0f;
        source->push_back(point);
    }
    source->width = static_cast<std::uint32_t>(source->size());
    source->height = 1;
    source->is_dense = true;
    return source;
}

pcl::PolygonMesh makeColorTransferMesh()
{
    pcl::PointCloud<pcl::PointXYZ> vertices;
    vertices.push_back(pcl::PointXYZ(0.0f, 0.0f, 0.0f));
    vertices.push_back(pcl::PointXYZ(1.0f, 1.0f, 0.0f));
    vertices.push_back(pcl::PointXYZ(1.0f, 0.0f, 0.0f));
    vertices.width = static_cast<std::uint32_t>(vertices.size());
    vertices.height = 1;

    pcl::PolygonMesh mesh;
    pcl::toPCLPointCloud2(vertices, mesh.cloud);
    pcl::Vertices triangle;
    triangle.vertices = {0, 1, 2};
    mesh.polygons.push_back(triangle);
    return mesh;
}

void testPoissonColorTransferUsesSpatialNeighbors()
{
    const pcl::PolygonMesh geometry = makeColorTransferMesh();
    pcl::PolygonMesh colored;
    expect(transferPoissonMeshColors(
        geometry, makeColorTransferSource(), colored),
        "Poisson color transfer must succeed for a valid mesh and source cloud");
    expect(hasMeshField(colored, "rgb"),
        "Poisson color transfer must add the packed RGB field");
    expect(meshVertexCount(colored) == meshVertexCount(geometry),
        "Poisson color transfer must preserve vertex count");
    expect(colored.polygons.size() == geometry.polygons.size()
            && colored.polygons[0].vertices
                == geometry.polygons[0].vertices,
        "Poisson color transfer must preserve polygon indices");

    pcl::PointCloud<pcl::PointXYZRGB> points;
    pcl::fromPCLPointCloud2(colored.cloud, points);
    expect(points[0].r == 255 && points[0].g == 0 && points[0].b == 0,
        "an exact coordinate match must copy the exact source color");
    expect(std::abs(static_cast<int>(points[1].r) - 85) <= 1
            && std::abs(static_cast<int>(points[1].g) - 85) <= 1
            && std::abs(static_cast<int>(points[1].b) - 85) <= 1,
        "three equidistant neighbors must produce their average color");
}

void testPoissonColorTransferRejectsEmptySource()
{
    pcl::PolygonMesh colored;
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr empty(
        new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    expect(!transferPoissonMeshColors(
        makeColorTransferMesh(), empty, colored),
        "Poisson color transfer must reject an empty source cloud");
    expect(colored.cloud.data.empty() && colored.polygons.empty(),
        "failed Poisson color transfer must not return a partial mesh");
}
```

Call both tests at the start of `main()`.

- [ ] **Step 2: Build and run to verify RED**

Run:

```powershell
cmd.exe /d /s /c 'call "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cd /d C:\wzp\Holographicface\mergeholo\vendor\point_cloud\tests && "C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe" mesh_reconstruction_tests.pro "CONFIG+=release" -o Makefile && nmake /NOLOGO /f Makefile.Release'
```

Expected: link fails with an unresolved external symbol for
`transferPoissonMeshColors`. This proves the tests exercise the missing
implementation.

- [ ] **Step 3: Implement the minimal color-transfer algorithm**

Add these includes to `poissonmesh.cpp`:

```cpp
#include <algorithm>
#include <cmath>
```

Implement the declared function before the crop function:

```cpp
bool transferPoissonMeshColors(
    const pcl::PolygonMesh& inputMesh,
    const pcl::PointCloud<pcl::PointXYZRGBNormal>::ConstPtr& sourceCloud,
    pcl::PolygonMesh& outputMesh)
{
    outputMesh = pcl::PolygonMesh();
    if (!sourceCloud || sourceCloud->empty()
        || inputMesh.cloud.data.empty() || inputMesh.polygons.empty()) {
        return false;
    }

    pcl::PointCloud<pcl::PointXYZ> meshPoints;
    pcl::fromPCLPointCloud2(inputMesh.cloud, meshPoints);
    if (meshPoints.empty()) {
        return false;
    }

    pcl::KdTreeFLANN<pcl::PointXYZRGBNormal> tree;
    tree.setInputCloud(sourceCloud);
    const int neighborCount = static_cast<int>(
        std::min<std::size_t>(3, sourceCloud->size()));
    constexpr float exactMatchSquaredDistance = 1.0e-12f;

    pcl::PointCloud<pcl::PointXYZRGB> coloredPoints;
    coloredPoints.reserve(meshPoints.size());
    coloredPoints.header = meshPoints.header;
    coloredPoints.is_dense = meshPoints.is_dense;

    std::vector<int> neighborIndices(neighborCount);
    std::vector<float> squaredDistances(neighborCount);
    for (const pcl::PointXYZ& meshPoint : meshPoints) {
        pcl::PointXYZRGBNormal query;
        query.x = meshPoint.x;
        query.y = meshPoint.y;
        query.z = meshPoint.z;
        const int found = tree.nearestKSearch(
            query, neighborCount, neighborIndices, squaredDistances);
        if (found <= 0) {
            return false;
        }

        pcl::PointXYZRGB colored;
        colored.x = meshPoint.x;
        colored.y = meshPoint.y;
        colored.z = meshPoint.z;
        colored.a = 255;
        if (squaredDistances[0] <= exactMatchSquaredDistance) {
            const pcl::PointXYZRGBNormal& source =
                (*sourceCloud)[neighborIndices[0]];
            colored.r = source.r;
            colored.g = source.g;
            colored.b = source.b;
        } else {
            double weightSum = 0.0;
            double red = 0.0;
            double green = 0.0;
            double blue = 0.0;
            for (int i = 0; i < found; ++i) {
                const double weight =
                    1.0 / std::sqrt(static_cast<double>(squaredDistances[i]));
                const pcl::PointXYZRGBNormal& source =
                    (*sourceCloud)[neighborIndices[i]];
                weightSum += weight;
                red += weight * source.r;
                green += weight * source.g;
                blue += weight * source.b;
            }
            if (!(weightSum > 0.0)) {
                return false;
            }
            colored.r = static_cast<unsigned char>(std::clamp(
                std::lround(red / weightSum), 0L, 255L));
            colored.g = static_cast<unsigned char>(std::clamp(
                std::lround(green / weightSum), 0L, 255L));
            colored.b = static_cast<unsigned char>(std::clamp(
                std::lround(blue / weightSum), 0L, 255L));
        }
        coloredPoints.push_back(colored);
    }

    coloredPoints.width = static_cast<std::uint32_t>(coloredPoints.size());
    coloredPoints.height = 1;
    outputMesh.header = inputMesh.header;
    outputMesh.polygons = inputMesh.polygons;
    pcl::toPCLPointCloud2(coloredPoints, outputMesh.cloud);
    outputMesh.cloud.header = inputMesh.cloud.header;
    return !outputMesh.cloud.data.empty()
        && coloredPoints.size() == meshPoints.size();
}
```

- [ ] **Step 4: Rebuild and run to verify GREEN**

Run the Task 1 Step 2 build command, then:

```powershell
$env:PATH = "C:\wzp\QT\5.15.0\msvc2019_64\bin;C:\wzp\Holographicface\mergeholo\00-bin;$env:PATH"
Push-Location C:\wzp\Holographicface\mergeholo\vendor\point_cloud\tests
.\release\mesh_reconstruction_tests.exe
Pop-Location
```

Expected: exit code `0`, output ends with
`mesh reconstruction tests passed`.

- [ ] **Step 5: Commit the focused helper**

```powershell
git add -- vendor/point_cloud/include/poissonmesh.hpp vendor/point_cloud/src/poissonmesh.cpp vendor/point_cloud/tests/test_mesh_reconstruction.cpp
git commit -m "feat: transfer RGB onto Poisson mesh vertices"
```

### Task 2: Wire RGB transfer into Poisson reconstruction

**Files:**
- Modify: `vendor/point_cloud/src/ConverPointCloud.cpp`
- Test: `vendor/point_cloud/tests/test_mesh_reconstruction.cpp`

**Interfaces:**
- Consumes: `transferPoissonMeshColors(...)` from Task 1.
- Produces: every successful `createPoissonMeshFromCloud(...)` result has a packed `rgb` field and every persisted Poisson PLY has `uchar red`, `uchar green`, and `uchar blue`.

- [ ] **Step 1: Write a failing end-to-end Poisson RGB test**

Add:

```cpp
void testPoissonReconstructionReturnsAndPersistsRgb()
{
    const TempDirectory temp;
    const fs::path output = temp.path() / "colored_mesh.ply";
    pcl::PolygonMesh mesh;
    ConverPointCloud converter;
    expect(converter.meshAPIFromCloud(
        makeSphereCloud(),
        (temp.path() / "colored_rgb.ply").string(),
        temp.writeConfig(1).string(),
        temp.path().string(),
        &mesh,
        false),
        "Poisson RGB integration reconstruction must succeed");
    expect(hasMeshField(mesh, "rgb"),
        "successful Poisson reconstruction must return RGB vertices");
    expect(pcl::io::savePLYFile(output.string(), mesh) == 0,
        "colored Poisson mesh must persist as PLY");
    const std::string ply = readProjectFile(output);
    expect(ply.find("property uchar red") != std::string::npos
            && ply.find("property uchar green") != std::string::npos
            && ply.find("property uchar blue") != std::string::npos,
        "persisted Poisson PLY must expose red, green, and blue properties");
}
```

Extend `readProjectFile` so an absolute path is checked first:

```cpp
std::string readProjectFile(const fs::path& relativePath)
{
    const fs::path candidates[] = {
        relativePath,
        fs::current_path() / relativePath,
        fs::current_path() / ".." / ".." / ".." / relativePath
    };
```

Call the test after the focused color-transfer tests in `main()`.

- [ ] **Step 2: Build and run to verify RED**

Use the Task 1 build and test commands.

Expected: executable exits `1` with
`check failed: successful Poisson reconstruction must return RGB vertices`.

- [ ] **Step 3: Wire transfer after crop with fail-closed behavior**

Replace the final crop block in `createPoissonMeshFromCloud` with:

```cpp
    pcl::PolygonMesh croppedMesh;
    if (!cropPoissonMeshToPointCloudHull(
            poissonMesh, cloudWithNormals, croppedMesh)) {
        cout << COUT_PREFIX
             << "Poisson crop-hull post-processing failed." << endl;
        return false;
    }
    if (!transferPoissonMeshColors(
            croppedMesh, cloudWithNormals, meshOut)) {
        meshOut = pcl::PolygonMesh();
        cout << COUT_PREFIX
             << "Poisson color transfer failed." << endl;
        return false;
    }
    return true;
```

- [ ] **Step 4: Rebuild and run to verify GREEN**

Use the Task 1 build and test commands.

Expected: exit code `0`; both the focused transfer tests and full
Poisson integration test pass.

- [ ] **Step 5: Commit the integration**

```powershell
git add -- vendor/point_cloud/src/ConverPointCloud.cpp vendor/point_cloud/tests/test_mesh_reconstruction.cpp
git commit -m "feat: preserve RGB in Poisson reconstruction"
```

### Task 3: Full regression, official build, and real-cloud benchmark

**Files:**
- Verify only: all six existing test targets.
- Build: `00-bin/mergeholo.exe`.
- Runtime artifacts: create the ignored directory
  `runs/poisson_rgb_20260723/`.

**Interfaces:**
- Consumes: completed Tasks 1 and 2.
- Produces: verified official executable and a real colored Poisson PLY with measured runtime.

- [ ] **Step 1: Run all six regression executables from their own working directories**

Use Qt before deployed runtime DLLs:

```powershell
$env:QT_QPA_PLATFORM = "offscreen"
$env:PATH = "C:\wzp\QT\5.15.0\msvc2019_64\bin;C:\wzp\Holographicface\mergeholo\00-bin;$env:PATH"
$tests = @(
    @{ Dir = "camera\tests"; Exe = "release\capture_orientation_tests.exe" },
    @{ Dir = "pipeline\tests"; Exe = "release\result_persistence_tests.exe" },
    @{ Dir = "printing\tests"; Exe = "release\printing_tests.exe" },
    @{ Dir = "widgets\tests"; Exe = "release\save_settings_dialog_tests.exe" },
    @{ Dir = "widgets\tests"; Exe = "release\processing_settings_tests.exe" },
    @{ Dir = "vendor\point_cloud\tests"; Exe = "release\mesh_reconstruction_tests.exe" }
)
foreach ($test in $tests) {
    Push-Location $test.Dir
    try {
        & ".\$($test.Exe)"
        if ($LASTEXITCODE -ne 0) {
            throw "$($test.Exe) failed with exit code $LASTEXITCODE"
        }
    } finally {
        Pop-Location
    }
}
```

Expected: all six exit `0`.

- [ ] **Step 2: Enforce the official rebuild rule**

```powershell
Get-Process -Name mergeholo -ErrorAction SilentlyContinue |
    Stop-Process -Force
if (Test-Path -LiteralPath "00-bin\mergeholo_verify.exe") {
    Remove-Item -LiteralPath "00-bin\mergeholo_verify.exe" -Force
}
.\scripts\build.ps1 -Config release -SkipDeploy
```

Expected: build exits `0`, linker writes `00-bin\mergeholo.exe`, and
`00-bin\mergeholo_verify.exe` is absent.

- [ ] **Step 3: Smoke-test the official executable**

```powershell
.\00-bin\mergeholo.exe --mergeholo-help
if ($LASTEXITCODE -ne 0) {
    throw "official mergeholo.exe smoke test failed"
}
```

Expected: exit code `0` and MergeHolo usage text.

- [ ] **Step 4: Run the real face point cloud through Poisson**

Create `runs/poisson_rgb_20260723/mesh.cfg` with:

```text
reconstruct=1
kSearch=20
searchradius=0.01
mu=2.5
maximumNearestNeighbors=100
maximumSurfaceAngle=45
minimumAngle=10
maximumAngle=120
holesize=0.005
focus=2000
leafsize=0.001
mlsSearchRadius=0.01
normalsFitIter1=1
normalsFitIter2=1
neighbor_num=20
nearest_distance=0.01
```

Create `runs/poisson_rgb_20260723/pipeline.ini` with:

```ini
depth_input_dir=C:/wzp/Holographicface/mergeholo/samples/face_roate
depth_config=C:/wzp/Holographicface/mergeholo/config/depth_to_pointcloud_config.cfg
mesh_config=C:/wzp/Holographicface/mergeholo/runs/poisson_rgb_20260723/mesh.cfg
mesh_obj=
output_root=C:/wzp/Holographicface/mergeholo/runs/poisson_rgb_20260723
multiview_out_dir=multiview
elemental_out_dir=elemental
log_file=C:/wzp/Holographicface/mergeholo/runs/poisson_rgb_20260723/pipeline.log
model_type=obj
run_depth_pointcloud=false
run_mesh=true
run_textured_model=false
run_multiview=false
run_elemental=false
```

Run:

```powershell
$watch = [System.Diagnostics.Stopwatch]::StartNew()
.\00-bin\mergeholo.exe --pipeline --config .\runs\poisson_rgb_20260723\pipeline.ini --stage mesh
$exitCode = $LASTEXITCODE
$watch.Stop()
if ($exitCode -ne 0) {
    throw "real Poisson RGB run failed with exit code $exitCode"
}
"elapsed_seconds=$($watch.Elapsed.TotalSeconds)"
```

Expected: exit `0` and `runs\poisson_rgb_20260723\0_mesh.ply` exists.

- [ ] **Step 5: Verify real PLY fields and summarize cost**

Run:

```powershell
rg -n -m 12 "^(element vertex|element face|property)" .\runs\poisson_rgb_20260723\0_mesh.ply
Get-Item -LiteralPath .\runs\poisson_rgb_20260723\0_mesh.ply |
    Select-Object FullName,Length,LastWriteTime
```

Expected header contains:

```text
property uchar red
property uchar green
property uchar blue
```

Record total time, vertex count, face count, and file size next to the
previous colorless baseline (`21.174 s`, `15,186` vertices, `29,530`
faces, `999,561` bytes).

- [ ] **Step 6: Final diff and repository verification**

```powershell
git diff --check
git status --short
git log -3 --oneline
Test-Path -LiteralPath "00-bin\mergeholo.exe"
Test-Path -LiteralPath "00-bin\mergeholo_verify.exe"
```

Expected: no whitespace errors; only intended source/test commits are new;
official executable exists; verify executable returns `False`.

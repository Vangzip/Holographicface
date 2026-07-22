# Mesh Reconstruction I/O Decoupling Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make Poisson reconstruction selectable in the live pipeline and make both Poisson and Greedy Projection Triangulation pure in-memory reconstruction modules, with file loading and mesh saving handled by adapters.

**Architecture:** `ConverPointCloud` remains the legacy orchestration boundary: public file and memory APIs parse configuration, adapt their input to an immutable PCL cloud, call one shared dispatcher, and optionally save the returned mesh. Algorithm functions accept only an in-memory cloud and output mesh. `poissonmesh` exposes a pure in-memory convex-hull crop helper instead of saving a PLY internally.

**Tech Stack:** C++17, PCL 1.12.1-rc1 (`NormalEstimation`, `Poisson`, `GreedyProjectionTriangulation`, `ConvexHull`, `CropHull`, `SimplificationRemoveUnusedVertices`), Qt 5.15 qmake, MSVC 2019 v142 x64, hand-written console tests.

## Global Constraints

- `reconstruct=1` means Poisson reconstruction.
- `reconstruct=2` means Greedy Projection Triangulation and remains the default.
- Reconstruction functions must not receive file paths, output directories, or write flags.
- The default camera pipeline passes point clouds and meshes entirely in memory.
- File input and optional PLY persistence must call the same in-memory dispatcher and must not duplicate either algorithm.
- Poisson failure must not silently fall back to Greedy.
- Preserve the existing Poisson parameters: confidence enabled, degree `2`, depth `8`, iso divide `8`, manifold enabled, output polygons disabled, samples per node `3`, scale `1.1`, solver divide `8`, and point weight `4.0`.
- Do not add Poisson tuning controls to the UI in this change.
- Preserve unrelated dirty-worktree changes and stage only reviewed feature hunks.
- Before editing, back up all affected existing source/header files outside the repository.
- Stop a running `mergeholo.exe` before the official build; delete `00-bin/mergeholo_verify.exe`; build only `00-bin/mergeholo.exe`.

## File Structure

- Create `vendor/point_cloud/tests/mesh_reconstruction_tests.pro`: isolated qmake target for the legacy point-cloud reconstruction boundary.
- Create `vendor/point_cloud/tests/test_mesh_reconstruction.cpp`: synthetic-cloud integration tests for dispatch, pure-memory behavior, output adaptation, and file/memory equivalence.
- Modify `vendor/point_cloud/include/ConverPointCloud.h`: declare pure in-memory algorithm functions and shared input/output adapters.
- Modify `vendor/point_cloud/src/ConverPointCloud.cpp`: remove duplicated file-oriented algorithms, add shared dispatch, and connect both public workflows.
- Modify `vendor/point_cloud/include/poissonmesh.hpp`: declare the stateless in-memory Poisson crop helper.
- Modify `vendor/point_cloud/src/poissonmesh.cpp`: implement cropping without path construction or serialization; retain legacy class APIs only for compatibility.
- Verify `mergeholo.pro`: no new production source is required; the existing `ConverPointCloud.cpp` and `poissonmesh.cpp` entries remain the only production compilation units.

---

### Task 1: Back up the legacy implementation and add the red integration test

**Files:**
- Create: `vendor/point_cloud/tests/mesh_reconstruction_tests.pro`
- Create: `vendor/point_cloud/tests/test_mesh_reconstruction.cpp`
- Back up: `vendor/point_cloud/include/ConverPointCloud.h`
- Back up: `vendor/point_cloud/src/ConverPointCloud.cpp`
- Back up: `vendor/point_cloud/include/poissonmesh.hpp`
- Back up: `vendor/point_cloud/src/poissonmesh.cpp`

**Interfaces:**
- Consumes existing public API: `bool ConverPointCloud::meshAPIFromCloud(const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr&, const string&, const string&, const string&, pcl::PolygonMesh*, bool)`.
- Produces test helper: `pcl::PointCloud<pcl::PointXYZRGB>::Ptr makeSphereCloud()`.
- Produces test contract: both reconstruction selector values return a non-empty `pcl::PolygonMesh` with `writeMeshFile=false` and create no PLY output.

- [ ] **Step 1: Create and verify a recoverable source backup**

Run from `C:\wzp\Holographicface\mergeholo`:

```powershell
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$backupRoot = "C:\wzp\Holographicface\mergeholo_backups\$stamp-poisson-reconstruction"
$resolvedProject = [System.IO.Path]::GetFullPath((Get-Location).Path)
if ($resolvedProject -ne 'C:\wzp\Holographicface\mergeholo') { throw "Unexpected project root: $resolvedProject" }
New-Item -ItemType Directory -Path "$backupRoot\vendor\point_cloud\include" -Force | Out-Null
New-Item -ItemType Directory -Path "$backupRoot\vendor\point_cloud\src" -Force | Out-Null
Copy-Item -LiteralPath 'vendor\point_cloud\include\ConverPointCloud.h' -Destination "$backupRoot\vendor\point_cloud\include\ConverPointCloud.h"
Copy-Item -LiteralPath 'vendor\point_cloud\include\poissonmesh.hpp' -Destination "$backupRoot\vendor\point_cloud\include\poissonmesh.hpp"
Copy-Item -LiteralPath 'vendor\point_cloud\src\ConverPointCloud.cpp' -Destination "$backupRoot\vendor\point_cloud\src\ConverPointCloud.cpp"
Copy-Item -LiteralPath 'vendor\point_cloud\src\poissonmesh.cpp' -Destination "$backupRoot\vendor\point_cloud\src\poissonmesh.cpp"
Get-FileHash -Algorithm SHA256 "$backupRoot\vendor\point_cloud\include\ConverPointCloud.h", "$backupRoot\vendor\point_cloud\include\poissonmesh.hpp", "$backupRoot\vendor\point_cloud\src\ConverPointCloud.cpp", "$backupRoot\vendor\point_cloud\src\poissonmesh.cpp"
```

Expected: four SHA-256 records and no files inside the Git worktree are changed.

- [ ] **Step 2: Add the isolated qmake test target**

Create `vendor/point_cloud/tests/mesh_reconstruction_tests.pro`:

```qmake
QT += core
QT -= gui widgets
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = mesh_reconstruction_tests

DEFINES += NOMINMAX _CRT_SECURE_NO_WARNINGS _HAS_STD_BYTE=0
win32:QMAKE_CXXFLAGS += /source-charset:utf-8 /execution-charset:utf-8

INCLUDEPATH += \
    ../../base \
    ../include

SOURCES += \
    test_mesh_reconstruction.cpp \
    ../../base/FileLibrary.cpp \
    ../../base/Logger.cpp \
    ../src/ConverPointCloud.cpp \
    ../src/modifiedPclFunctions.cpp \
    ../src/OdmTexturing.cpp \
    ../src/poissonmesh.cpp

HEADERS += \
    ../include/ConverPointCloud.h \
    ../include/poissonmesh.hpp

include(../../../Pri/opencv.pri)
include(../../../Pri/holo_pipeline.pri)

win32:LIBS += -limagehlp
```

`vendor/base/base.h` already contains the MSVC `#pragma comment(lib, ...)` declarations for the PCL and VTK libraries used by these production sources; do not duplicate the legacy full library list in this test project.

- [ ] **Step 3: Write the failing memory-path tests**

Create `vendor/point_cloud/tests/test_mesh_reconstruction.cpp` with these helpers and cases:

```cpp
#include "ConverPointCloud.h"

#include <pcl/io/ply_io.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <process.h>
#endif

namespace fs = std::filesystem;

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "check failed: " << message << '\n';
        std::exit(1);
    }
}

class TempDirectory {
public:
    TempDirectory()
    {
        const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
#ifdef _WIN32
        const int pid = _getpid();
#else
        const int pid = 0;
#endif
        path_ = fs::temp_directory_path()
            / ("mergeholo_mesh_reconstruction_" + std::to_string(pid)
                + "_" + std::to_string(ticks));
        fs::create_directories(path_);
    }

    ~TempDirectory()
    {
        std::error_code error;
        fs::remove_all(path_, error);
    }

    fs::path writeConfig(int reconstruct) const
    {
        const fs::path path = path_ / ("mesh_" + std::to_string(reconstruct) + ".cfg");
        std::ofstream output(path);
        output
            << "reconstruct=" << reconstruct << "\n"
            << "kSearch=20\n"
            << "searchradius=0.012\n"
            << "mu=2.5\n"
            << "maximumNearestNeighbors=100\n"
            << "maximumSurfaceAngle=45\n"
            << "minimumAngle=10\n"
            << "maximumAngle=120\n"
            << "holesize=0\n"
            << "focus=2000\n"
            << "leafsize=0\n"
            << "mlsSearchRadius=0.01\n"
            << "normalsFitIter1=1\n"
            << "normalsFitIter2=1\n"
            << "neighbor_num=20\n"
            << "nearest_distance=0.01\n";
        return path;
    }

    const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

pcl::PointCloud<pcl::PointXYZRGB>::Ptr makeSphereCloud()
{
    constexpr double pi = 3.14159265358979323846;
    constexpr int latitudeCount = 36;
    constexpr int longitudeCount = 72;
    constexpr float radius = 0.05f;
    constexpr float centerZ = 0.15f;

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(
        new pcl::PointCloud<pcl::PointXYZRGB>);
    cloud->reserve((latitudeCount - 1) * longitudeCount);
    for (int latitude = 1; latitude < latitudeCount; ++latitude) {
        const double phi = pi * latitude / latitudeCount;
        for (int longitude = 0; longitude < longitudeCount; ++longitude) {
            const double theta = 2.0 * pi * longitude / longitudeCount;
            pcl::PointXYZRGB point;
            point.x = radius * static_cast<float>(std::sin(phi) * std::cos(theta));
            point.y = radius * static_cast<float>(std::sin(phi) * std::sin(theta));
            point.z = centerZ + radius * static_cast<float>(std::cos(phi));
            point.r = static_cast<unsigned char>(40 + latitude * 4);
            point.g = static_cast<unsigned char>(40 + longitude * 2);
            point.b = 180;
            cloud->push_back(point);
        }
    }
    cloud->width = static_cast<std::uint32_t>(cloud->size());
    cloud->height = 1;
    cloud->is_dense = true;
    return cloud;
}

void expectNoMeshFile(const fs::path& directory)
{
    for (const fs::directory_entry& entry : fs::directory_iterator(directory)) {
        expect(entry.path().filename() != "capture_mesh.ply",
            "memory-only reconstruction must not save capture_mesh.ply");
    }
}

void testMemoryReconstruction(int method)
{
    const TempDirectory temp;
    const fs::path config = temp.writeConfig(method);
    pcl::PolygonMesh mesh;
    ConverPointCloud converter;
    const bool ok = converter.meshAPIFromCloud(
        makeSphereCloud(),
        (temp.path() / "capture_rgb.ply").string(),
        config.string(),
        temp.path().string(),
        &mesh,
        false);
    expect(ok, method == 1
        ? "Poisson memory reconstruction must succeed"
        : "Greedy memory reconstruction must succeed");
    expect(!mesh.cloud.data.empty(), "memory mesh must contain vertices");
    expect(!mesh.polygons.empty(), "memory mesh must contain polygons");
    expectNoMeshFile(temp.path());
}

void testUnknownMethodIsRejected()
{
    const TempDirectory temp;
    pcl::PolygonMesh mesh;
    ConverPointCloud converter;
    expect(!converter.meshAPIFromCloud(
        makeSphereCloud(),
        (temp.path() / "capture_rgb.ply").string(),
        temp.writeConfig(99).string(),
        temp.path().string(),
        &mesh,
        false),
        "unknown reconstruction method must fail");
}

} // namespace

int main()
{
    testMemoryReconstruction(2);
    testMemoryReconstruction(1);
    testUnknownMethodIsRejected();
    std::cout << "mesh reconstruction tests passed\n";
    return 0;
}
```

- [ ] **Step 4: Build and run the test to verify the Poisson case is red**

Run:

```powershell
cmd.exe /d /s /c 'call "C:\wzp\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cd /d C:\wzp\Holographicface\mergeholo\vendor\point_cloud\tests && "C:\wzp\QT\5.15.0\msvc2019_64\bin\qmake.exe" mesh_reconstruction_tests.pro "CONFIG+=release" -o Makefile && nmake /NOLOGO /f Makefile.Release'
$runtime = @(
    'C:\wzp\Holographicface\mergeholo\00-bin',
    'C:\wzp\Holographicface\PCL 1.12.1-rc1\bin',
    'C:\wzp\Holographicface\PCL 1.12.1-rc1\3rdParty\VTK\bin',
    'C:\wzp\Holographicface\PCL 1.12.1-rc1\3rdParty\FLANN\bin',
    'C:\wzp\Holographicface\PCL 1.12.1-rc1\3rdParty\Qhull\bin',
    'C:\wzp\Holographicface\opencv450\opencv\build\x64\vc15\bin',
    'C:\wzp\Holographicface\OSG365\bin',
    'C:\wzp\Holographicface\OE32\bin',
    'C:\wzp\QT\5.15.0\msvc2019_64\bin'
)
$env:PATH = ($runtime -join ';') + ';' + $env:PATH
vendor\point_cloud\tests\release\mesh_reconstruction_tests.exe
```

Expected: Greedy succeeds; Poisson fails with `Memory mesh path currently supports GreedyProjectionTriangulation only.` The test process exits non-zero before printing the pass message.

---

### Task 2: Convert Poisson reconstruction and crop-hull post-processing to pure memory

**Files:**
- Modify: `vendor/point_cloud/include/poissonmesh.hpp`
- Modify: `vendor/point_cloud/src/poissonmesh.cpp`
- Modify: `vendor/point_cloud/include/ConverPointCloud.h`
- Modify: `vendor/point_cloud/src/ConverPointCloud.cpp`
- Test: `vendor/point_cloud/tests/test_mesh_reconstruction.cpp`

**Interfaces:**
- Produces: `bool cropPoissonMeshToPointCloudHull(const pcl::PolygonMesh&, const pcl::PointCloud<pcl::PointXYZRGBNormal>::ConstPtr&, pcl::PolygonMesh&)`.
- Produces private method: `bool ConverPointCloud::createPoissonMeshFromCloud(const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr&, pcl::PolygonMesh&)`.
- Consumes existing configuration members including `m_leafsize`.
- Does not construct a path or call `pcl::io::savePLYFile`.

- [ ] **Step 1: Declare the stateless crop helper and pure Poisson method**

Add to `poissonmesh.hpp` before `texturemeshPoisson`:

```cpp
bool cropPoissonMeshToPointCloudHull(
    const pcl::PolygonMesh& inputMesh,
    const pcl::PointCloud<pcl::PointXYZRGBNormal>::ConstPtr& sourceCloud,
    pcl::PolygonMesh& outputMesh);
```

Replace the private file-oriented declaration in `ConverPointCloud.h`:

```cpp
bool createPoissonMeshFromCloud(
    const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr& cloud,
    pcl::PolygonMesh& meshOut);
```

- [ ] **Step 2: Implement crop-hull as a pure in-memory transformation**

Implement `cropPoissonMeshToPointCloudHull` in `poissonmesh.cpp` using local state:

```cpp
bool cropPoissonMeshToPointCloudHull(
    const pcl::PolygonMesh& inputMesh,
    const pcl::PointCloud<pcl::PointXYZRGBNormal>::ConstPtr& sourceCloud,
    pcl::PolygonMesh& outputMesh)
{
    outputMesh = pcl::PolygonMesh();
    if (!sourceCloud || sourceCloud->empty()
        || inputMesh.cloud.data.empty() || inputMesh.polygons.empty()) {
        return false;
    }

    pcl::PointCloud<pcl::PointXYZ>::Ptr meshPoints(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(inputMesh.cloud, *meshPoints);
    if (meshPoints->empty()) {
        return false;
    }

    pcl::PointCloud<pcl::PointXYZ>::Ptr sourcePoints(new pcl::PointCloud<pcl::PointXYZ>);
    sourcePoints->resize(sourceCloud->size());
    sourcePoints->width = sourceCloud->width;
    sourcePoints->height = sourceCloud->height;
    sourcePoints->is_dense = sourceCloud->is_dense;
    for (std::size_t i = 0; i < sourceCloud->size(); ++i) {
        (*sourcePoints)[i].x = (*sourceCloud)[i].x;
        (*sourcePoints)[i].y = (*sourceCloud)[i].y;
        (*sourcePoints)[i].z = (*sourceCloud)[i].z;
    }

    pcl::PointCloud<pcl::PointXYZ>::Ptr hullPoints(new pcl::PointCloud<pcl::PointXYZ>);
    std::vector<pcl::Vertices> hullPolygons;
    std::unique_ptr<pcl::ConvexHull<pcl::PointXYZ>> hull(
        new pcl::ConvexHull<pcl::PointXYZ>);
    hull->setDimension(3);
    hull->setInputCloud(sourcePoints);
    hull->reconstruct(*hullPoints, hullPolygons);
    if (hullPoints->empty() || hullPolygons.empty()) {
        hull.reset();
        return false;
    }

    std::vector<int> insideIndices;
    std::unique_ptr<pcl::CropHull<pcl::PointXYZ>> crop(
        new pcl::CropHull<pcl::PointXYZ>);
    crop->setCropOutside(true);
    crop->setDim(3);
    crop->setInputCloud(meshPoints);
    crop->setHullIndices(hullPolygons);
    crop->setHullCloud(hullPoints);
    crop->filter(insideIndices);

    std::vector<unsigned char> inside(meshPoints->size(), 0);
    for (int index : insideIndices) {
        if (index >= 0 && static_cast<std::size_t>(index) < inside.size()) {
            inside[static_cast<std::size_t>(index)] = 1;
        }
    }

    pcl::PolygonMesh visible = inputMesh;
    visible.polygons.clear();
    visible.polygons.reserve(inputMesh.polygons.size());
    for (const pcl::Vertices& face : inputMesh.polygons) {
        bool keep = face.vertices.size() >= 3;
        for (std::uint32_t index : face.vertices) {
            if (index >= inside.size() || inside[index] == 0) {
                keep = false;
                break;
            }
        }
        if (keep) {
            visible.polygons.push_back(face);
        }
    }

    crop.reset();
    hull.reset();
    if (visible.polygons.empty()) {
        return false;
    }

    pcl::surface::SimplificationRemoveUnusedVertices cleaner;
    cleaner.simplify(visible, outputMesh);
    return !outputMesh.cloud.data.empty() && !outputMesh.polygons.empty();
}
```

Add these includes to `poissonmesh.cpp`:

```cpp
#include <cstdint>
#include <memory>
#include <vector>
```

Keep the legacy class overloads compiling, but the new production reconstruction path must not call an overload that saves `m_strPolygonMeshFile`.

- [ ] **Step 3: Extract the Poisson core from `createPoissonMesh`**

Replace the body of the file-oriented Poisson function with this pure function. This retains the current filter and normal-estimation behavior and makes every input and output explicit:

```cpp
bool ConverPointCloud::createPoissonMeshFromCloud(
    const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr& inputCloud,
    pcl::PolygonMesh& meshOut)
{
    meshOut = pcl::PolygonMesh();
    if (!inputCloud || inputCloud->empty()) {
        cout << COUT_PREFIX << "Poisson input is empty." << endl;
        return false;
    }

    pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr filteredCloud = inputCloud;
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr voxelCloud(
        new pcl::PointCloud<pcl::PointXYZRGB>);
    if (m_leafsize != 0.0f) {
        pcl::VoxelGrid<pcl::PointXYZRGB> voxel;
        voxel.setInputCloud(filteredCloud);
        voxel.setLeafSize(m_leafsize, m_leafsize, m_leafsize);
        voxel.filter(*voxelCloud);
        if (voxelCloud->empty()) {
            cout << COUT_PREFIX << "Poisson voxel filtering produced no points." << endl;
            return false;
        }
        filteredCloud = voxelCloud;
    }

    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZRGB>::Ptr normalTree(
        new pcl::search::KdTree<pcl::PointXYZRGB>);
    normalTree->setInputCloud(filteredCloud);
    pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal> estimator;
    estimator.setInputCloud(filteredCloud);
    estimator.setSearchMethod(normalTree);
    estimator.setRadiusSearch(0.01);
    estimator.compute(*normals);
    if (normals->size() != filteredCloud->size()) {
        cout << COUT_PREFIX << "Poisson normal estimation size mismatch." << endl;
        return false;
    }

    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloudWithNormals(
        new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    cloudWithNormals->resize(filteredCloud->size());
    cloudWithNormals->width = filteredCloud->width;
    cloudWithNormals->height = filteredCloud->height;
    cloudWithNormals->is_dense = filteredCloud->is_dense;
    for (std::size_t i = 0; i < filteredCloud->size(); ++i) {
        const pcl::PointXYZRGB& point = (*filteredCloud)[i];
        const pcl::Normal& normal = (*normals)[i];
        if (!pcl::isFinite(normal)) {
            cout << COUT_PREFIX
                 << "Poisson normal estimation produced a non-finite normal." << endl;
            return false;
        }
        pcl::PointXYZRGBNormal& combined = (*cloudWithNormals)[i];
        combined.x = point.x;
        combined.y = point.y;
        combined.z = point.z;
        combined.r = point.r;
        combined.g = point.g;
        combined.b = point.b;
        combined.normal_x = normal.normal_x;
        combined.normal_y = normal.normal_y;
        combined.normal_z = normal.normal_z;
    }

    pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree(
        new pcl::search::KdTree<pcl::PointXYZRGBNormal>);
    tree->setInputCloud(cloudWithNormals);

    pcl::PolygonMesh poissonMesh;
    pcl::Poisson<pcl::PointXYZRGBNormal> poisson;
    poisson.setConfidence(true);
    poisson.setDegree(2);
    poisson.setDepth(8);
    poisson.setIsoDivide(8);
    poisson.setManifold(true);
    poisson.setOutputPolygons(false);
    poisson.setSamplesPerNode(3);
    poisson.setScale(1.1);
    poisson.setSolverDivide(8);
    poisson.setPointWeight(4.0);
    poisson.setSearchMethod(tree);
    poisson.setInputCloud(cloudWithNormals);
    poisson.reconstruct(poissonMesh);
    if (poissonMesh.cloud.data.empty() || poissonMesh.polygons.empty()) {
        cout << COUT_PREFIX << "Poisson reconstruction produced an empty mesh." << endl;
        return false;
    }

    if (!cropPoissonMeshToPointCloudHull(poissonMesh, cloudWithNormals, meshOut)) {
        cout << COUT_PREFIX << "Poisson crop-hull post-processing failed." << endl;
        return false;
    }
    return true;
}
```

The function must not reference `filepath`, `srcfile`, `m_strOutModelPath`, `m_strMeshOutputDir`, `texturemeshPoisson`, or `savePLYFile`.

- [ ] **Step 4: Enable Poisson in the existing memory dispatcher**

Replace the `m_type != 2` rejection in `meshAPIFromCloud` with explicit dispatch:

```cpp
pcl::PolygonMesh reconstructed;
bool result = false;
if (m_type == 1) {
    result = createPoissonMeshFromCloud(cloud, reconstructed);
}
else if (m_type == 2) {
    result = createGreedMeshFromCloud(
        cloud, logicalFlypath, &reconstructed, false);
}
else {
    cout << COUT_PREFIX << "Error: Unknown reconstruction type: " << m_type << endl;
    return false;
}

if (!result) {
    return false;
}
if (writeMeshFile) {
    m_strOutModelPath = buildMeshOutputPath(logicalFlypath, "_mesh.ply");
    if (pcl::io::savePLYFile(m_strOutModelPath, reconstructed) != 0) {
        cout << COUT_PREFIX << "Mesh output failed: " << m_strOutModelPath << endl;
        return false;
    }
}
if (meshOut) {
    *meshOut = reconstructed;
}
return true;
```

This is an intermediate green step. Task 3 removes the remaining path/write arguments from the Greedy implementation and deduplicates file mode.

- [ ] **Step 5: Run the focused test and commit the working Poisson memory path**

Rebuild and run `mesh_reconstruction_tests.exe` with the Task 1 commands.

Expected: both `reconstruct=1` and `reconstruct=2` return non-empty meshes; no `capture_mesh.ply` exists; selector `99` fails; the test prints `mesh reconstruction tests passed` and exits `0`.

Review and commit only these files:

```powershell
git diff --check -- vendor/point_cloud/include/ConverPointCloud.h vendor/point_cloud/src/ConverPointCloud.cpp vendor/point_cloud/include/poissonmesh.hpp vendor/point_cloud/src/poissonmesh.cpp vendor/point_cloud/tests
git add -- vendor/point_cloud/include/ConverPointCloud.h vendor/point_cloud/src/ConverPointCloud.cpp vendor/point_cloud/include/poissonmesh.hpp vendor/point_cloud/src/poissonmesh.cpp vendor/point_cloud/tests/mesh_reconstruction_tests.pro vendor/point_cloud/tests/test_mesh_reconstruction.cpp
git commit -m "feat: add in-memory Poisson reconstruction"
```

---

### Task 3: Make both algorithms use the same input, dispatch, and output adapters

**Files:**
- Modify: `vendor/point_cloud/include/ConverPointCloud.h`
- Modify: `vendor/point_cloud/src/ConverPointCloud.cpp`
- Modify: `vendor/point_cloud/tests/test_mesh_reconstruction.cpp`

**Interfaces:**
- Produces private adapter: `bool loadPointCloud(const string&, pcl::PointCloud<pcl::PointXYZRGB>::Ptr&) const`.
- Produces private dispatcher: `bool reconstructMeshFromCloud(const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr&, pcl::PolygonMesh&)`.
- Produces private output adapter: `bool saveMesh(const pcl::PolygonMesh&, const string&) const`.
- Changes Greedy core to: `bool createGreedMeshFromCloud(const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr&, pcl::PolygonMesh&)`.
- Consumes Poisson core from Task 2 with the same input/output contract.

- [ ] **Step 1: Add a failing file/memory equivalence and persistence test**

Append to `test_mesh_reconstruction.cpp`:

```cpp
std::size_t meshVertexCount(const pcl::PolygonMesh& mesh)
{
    return static_cast<std::size_t>(mesh.cloud.width)
        * static_cast<std::size_t>(mesh.cloud.height);
}

void testGreedyFileAndMemoryAdaptersAreEquivalent()
{
    const TempDirectory temp;
    const fs::path config = temp.writeConfig(2);
    const auto cloud = makeSphereCloud();
    const fs::path input = temp.path() / "sample_rgb.ply";
    expect(pcl::io::savePLYFileBinary(input.string(), *cloud) == 0,
        "test input PLY must be saved");

    pcl::PolygonMesh memoryMesh;
    ConverPointCloud memoryConverter;
    expect(memoryConverter.meshAPIFromCloud(
        cloud, input.string(), config.string(), temp.path().string(),
        &memoryMesh, false),
        "Greedy memory adapter must succeed");

    ConverPointCloud fileConverter;
    expect(fileConverter.meshAPI(
        input.string(), config.string(), temp.path().string()),
        "Greedy file adapter must succeed");

    pcl::PolygonMesh fileMesh;
    const fs::path output = temp.path() / "sample_mesh.ply";
    expect(fs::exists(output), "file adapter must save sample_mesh.ply");
    expect(pcl::io::loadPLYFile(output.string(), fileMesh) == 0,
        "saved mesh must be readable");
    expect(meshVertexCount(fileMesh) == meshVertexCount(memoryMesh),
        "file and memory adapters must return the same vertex count");
    expect(fileMesh.polygons.size() == memoryMesh.polygons.size(),
        "file and memory adapters must return the same polygon count");
}

void testMemoryAdapterCanPersistReturnedMesh()
{
    const TempDirectory temp;
    pcl::PolygonMesh mesh;
    ConverPointCloud converter;
    expect(converter.meshAPIFromCloud(
        makeSphereCloud(),
        (temp.path() / "persist_rgb.ply").string(),
        temp.writeConfig(1).string(),
        temp.path().string(),
        &mesh,
        true),
        "Poisson memory adapter with persistence must succeed");
    expect(fs::exists(temp.path() / "persist_mesh.ply"),
        "memory adapter must save the already returned mesh when requested");
    expect(!mesh.polygons.empty(), "persisted memory mesh must still be returned");
}
```

Call both functions from `main()` after the selector tests. Before refactoring, the topology comparison may expose differences between the duplicated Greedy implementations; after refactoring, both paths are guaranteed to call the same function.

- [ ] **Step 2: Declare the shared adapters and pure dispatcher**

Replace the file-oriented private declarations in `ConverPointCloud.h` with:

```cpp
bool loadPointCloud(
    const string& path,
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr& cloud) const;
bool reconstructMeshFromCloud(
    const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr& cloud,
    pcl::PolygonMesh& meshOut);
bool saveMesh(const pcl::PolygonMesh& mesh, const string& path) const;

bool createPoissonMeshFromCloud(
    const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr& cloud,
    pcl::PolygonMesh& meshOut);
bool createGreedMeshFromCloud(
    const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr& cloud,
    pcl::PolygonMesh& meshOut);
```

Remove `createPoissonMesh(const string&)` and `createGreedMesh(const string&)` declarations.

- [ ] **Step 3: Implement the shared input, dispatch, and output functions**

Add to `ConverPointCloud.cpp`:

```cpp
bool ConverPointCloud::loadPointCloud(
    const string& path,
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr& cloud) const
{
    cloud.reset(new pcl::PointCloud<pcl::PointXYZRGB>);
    if (pcl::io::loadPLYFile(path, *cloud) != 0 || cloud->empty()) {
        cout << COUT_PREFIX << "Point-cloud input failed: " << path << endl;
        cloud.reset();
        return false;
    }
    return true;
}

bool ConverPointCloud::reconstructMeshFromCloud(
    const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr& cloud,
    pcl::PolygonMesh& meshOut)
{
    meshOut = pcl::PolygonMesh();
    if (!cloud || cloud->empty()) {
        cout << COUT_PREFIX << "Reconstruction input is empty." << endl;
        return false;
    }

    bool ok = false;
    if (m_type == 1) {
        cout << COUT_PREFIX << "Reconstruction type: Poisson" << endl;
        ok = createPoissonMeshFromCloud(cloud, meshOut);
    }
    else if (m_type == 2) {
        cout << COUT_PREFIX << "Reconstruction type: GreedyProjectionTriangulation" << endl;
        ok = createGreedMeshFromCloud(cloud, meshOut);
    }
    else {
        cout << COUT_PREFIX << "Unknown reconstruction type: " << m_type << endl;
        return false;
    }

    if (!ok || meshOut.cloud.data.empty() || meshOut.polygons.empty()) {
        cout << COUT_PREFIX << "Reconstruction produced no usable mesh." << endl;
        meshOut = pcl::PolygonMesh();
        return false;
    }
    return true;
}

bool ConverPointCloud::saveMesh(
    const pcl::PolygonMesh& mesh,
    const string& path) const
{
    if (mesh.cloud.data.empty() || mesh.polygons.empty()) {
        cout << COUT_PREFIX << "Mesh output rejected an empty mesh." << endl;
        return false;
    }
    if (pcl::io::savePLYFile(path, mesh) != 0) {
        cout << COUT_PREFIX << "Mesh output failed: " << path << endl;
        return false;
    }
    cout << COUT_PREFIX << "Mesh output saved: " << path << endl;
    return true;
}
```

- [ ] **Step 4: Make Greedy a pure algorithm function**

Change `createGreedMeshFromCloud` to the two-argument signature. Keep the existing code from its input validation through `gp3->reconstruct(*triangles)` byte-for-byte except for renaming the input parameter to `inputCloud`. Delete the current path/write block beginning with `m_strOutModelPath = buildMeshOutputPath(...)`, and use this complete replacement tail:

Add `#include <utility>` with the other includes at the top of `ConverPointCloud.cpp`.

```cpp
pcl::PolygonMesh postProcessed;
fillHole(*triangles, postProcessed);
if (postProcessed.cloud.data.empty() || postProcessed.polygons.empty()) {
    releaseGreedyProjection();
    ne.reset();
    return false;
}
meshOut = std::move(postProcessed);
releaseGreedyProjection();
ne.reset();
return true;
```

Delete both duplicated file-oriented bodies `createGreedMesh(const string&)` and `createPoissonMesh(const string&)` after their preprocessing and reconstruction logic is represented in the two pure functions.

- [ ] **Step 5: Rebuild both public workflows around the shared dispatcher**

Implement the public APIs as adapters:

```cpp
bool ConverPointCloud::meshAPI(
    const string& flypath,
    const string& config,
    const string& outputDir)
{
    if (!parseArguments(config)) {
        cout << COUT_PREFIX << "Configuration input failed: " << config << endl;
        return false;
    }
    m_strMeshOutputDir = outputDir;

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud;
    if (!loadPointCloud(flypath, cloud)) {
        return false;
    }

    pcl::PolygonMesh mesh;
    if (!reconstructMeshFromCloud(cloud, mesh)) {
        return false;
    }

    m_strOutModelPath = buildMeshOutputPath(flypath, "_mesh.ply");
    return saveMesh(mesh, m_strOutModelPath);
}

bool ConverPointCloud::meshAPIFromCloud(
    const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr& cloud,
    const string& logicalFlypath,
    const string& config,
    const string& outputDir,
    pcl::PolygonMesh* meshOut,
    bool writeMeshFile)
{
    if (!parseArguments(config)) {
        cout << COUT_PREFIX << "Configuration input failed: " << config << endl;
        return false;
    }
    m_strMeshOutputDir = outputDir;

    pcl::PolygonMesh mesh;
    if (!reconstructMeshFromCloud(cloud, mesh)) {
        return false;
    }

    m_strOutModelPath = buildMeshOutputPath(logicalFlypath, "_mesh.ply");
    if (writeMeshFile && !saveMesh(mesh, m_strOutModelPath)) {
        return false;
    }
    if (meshOut) {
        *meshOut = mesh;
    }
    return true;
}
```

The public memory adapter retains logical path/output arguments for backward compatibility and naming, but neither argument crosses the dispatcher boundary.

- [ ] **Step 6: Run focused tests, scan the algorithm bodies for I/O, and commit**

Rebuild and run `mesh_reconstruction_tests.exe`.

Expected: all memory tests, file/memory equivalence, and conditional persistence pass.

Run the structural check:

```powershell
rg -n "savePLYFile|loadPLYFile|buildMeshOutputPath|m_strMeshOutputDir|m_strOutModelPath" vendor/point_cloud/src/ConverPointCloud.cpp
```

Expected: matches occur only in `loadPointCloud`, `saveMesh`, `meshAPI`, `meshAPIFromCloud`, output naming, model code, or unrelated legacy utilities; no match occurs inside `createPoissonMeshFromCloud`, `createGreedMeshFromCloud`, or `reconstructMeshFromCloud`.

Commit:

```powershell
git diff --check -- vendor/point_cloud/include/ConverPointCloud.h vendor/point_cloud/src/ConverPointCloud.cpp vendor/point_cloud/tests/test_mesh_reconstruction.cpp
git add -- vendor/point_cloud/include/ConverPointCloud.h vendor/point_cloud/src/ConverPointCloud.cpp vendor/point_cloud/tests/test_mesh_reconstruction.cpp
git commit -m "refactor: decouple mesh reconstruction I/O"
```

---

### Task 4: Verify pipeline compatibility and build the official executable

**Files:**
- Modify only files needed to correct a verified test or build failure.

**Interfaces:**
- Consumes unchanged public `meshAPI` and `meshAPIFromCloud` signatures from `pipeline/stages/MeshStage.cpp`.
- Produces the official `C:\wzp\Holographicface\mergeholo\00-bin\mergeholo.exe` only.

- [ ] **Step 1: Verify the mesh-stage call still defaults to pure memory**

Inspect `pipeline/stages/MeshStage.cpp` and confirm that when `MeshMemoryResult` is requested it passes a non-null `meshOut` and `writeMeshFile=false`:

```cpp
const bool keepMeshInMemory = meshMemory != nullptr;
const bool ok = converter.meshAPIFromCloud(
    depthMemory->cloud,
    depthMemory->pointCloudPath.string(),
    config.meshConfig.string(),
    config.depthInputDir.string(),
    keepMeshInMemory ? &mesh : nullptr,
    !keepMeshInMemory);
```

Do not change this call unless compilation proves the public compatibility signature was accidentally altered.

- [ ] **Step 2: Run the focused point-cloud suite and existing regression suites**

Run:

```text
vendor/point_cloud/tests/release/mesh_reconstruction_tests.exe
pipeline/tests/release/result_persistence_tests.exe
widgets/tests/release/processing_settings_tests.exe
widgets/tests/release/save_settings_dialog_tests.exe
camera/tests/release/capture_orientation_tests.exe
printing/tests/release/printing_tests.exe
```

Expected: all six exit `0` and print their pass messages. The processing settings test must continue to preserve both `reconstruct=1` and `reconstruct=2` values; no UI edit is needed.

- [ ] **Step 3: Stop only the running official application**

Run:

```powershell
Get-Process -Name mergeholo -ErrorAction SilentlyContinue | Stop-Process -Force
if (Get-Process -Name mergeholo -ErrorAction SilentlyContinue) {
    throw 'mergeholo.exe is still running'
}
```

Expected: no `mergeholo` process remains before linking.

- [ ] **Step 4: Remove only the obsolete verification executable**

Resolve and validate the exact target before deletion:

```powershell
$verifyExe = [System.IO.Path]::GetFullPath('C:\wzp\Holographicface\mergeholo\00-bin\mergeholo_verify.exe')
$expectedVerifyExe = 'C:\wzp\Holographicface\mergeholo\00-bin\mergeholo_verify.exe'
if ($verifyExe -ne $expectedVerifyExe) { throw "Unexpected verification target: $verifyExe" }
if (Test-Path -LiteralPath $verifyExe) { Remove-Item -LiteralPath $verifyExe -Force }
if (Test-Path -LiteralPath $verifyExe) { throw 'mergeholo_verify.exe was not removed' }
```

- [ ] **Step 5: Build only the official release executable**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Config release
```

Expected: qmake and `nmake` exit `0`; linker output targets `00-bin\mergeholo.exe`; `00-bin\mergeholo_verify.exe` remains absent.

- [ ] **Step 6: Run final smoke and scope verification**

Run:

```powershell
.\00-bin\mergeholo.exe --mergeholo-help
git diff --check -- vendor/point_cloud
Test-Path -LiteralPath .\00-bin\mergeholo.exe
Test-Path -LiteralPath .\00-bin\mergeholo_verify.exe
```

Expected: help exits `0`; feature files have no whitespace errors; the official executable check is `True`; the verification executable check is `False`.

Manually select `泊松重建`, perform one representative camera capture, and confirm the log reaches model texturing with a non-empty in-memory mesh and no intermediate `*_rgb.ply` or `*_mesh.ply` unless result persistence is enabled. Repeat with `贪婪三角化` to verify the existing default behavior.

- [ ] **Step 7: Review final scope and commit any verification-only fix**

If verification required a source correction, rerun the failing suite and stage only that correction:

```powershell
git status --short
git diff -- vendor/point_cloud pipeline/stages/MeshStage.cpp mergeholo.pro
git add -p -- vendor/point_cloud pipeline/stages/MeshStage.cpp mergeholo.pro
git commit -m "fix: preserve mesh pipeline reconstruction compatibility"
```

If no correction was required, do not create an empty commit.

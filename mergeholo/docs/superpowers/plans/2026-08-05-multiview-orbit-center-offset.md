# Multiview Orbit-Center Offset Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the UI's “远近位置” (`centerZ`) the shared orbit center for every generated multiview image.

**Architecture:** Retain the camera target already installed by `modelMoveHandler`, then make both renderers derive their orbit basis from that current look-at target.  The mesh transform remains unchanged, so UI offsets keep their existing coordinate convention.  A compact OSG regression-test executable will use a deliberately off-center camera target and verify the generated matrices preserve it.

**Tech Stack:** C++17, Qt qmake, OpenSceneGraph 3.6.5, existing multiview render-plan classes.

## Global Constraints

- Do not change point-cloud filtering, mesh reconstruction, capture orientation, or FOV.
- Preserve a `centerZ` value of `0.0` as an orbit around the centered mesh origin.
- Keep the UI setting range and persistence unchanged; the user selects `+0.10` for the first trial.
- Apply the same camera target in both batch and atlas paths.

---

### Task 1: Extract and test the orbit-matrix calculation

**Files:**
- Create: `vendor/multiview/multiviewOrbitMatrices.h`
- Create: `vendor/multiview/tests/multiview_orbit_tests.pro`
- Create: `vendor/multiview/tests/test_multiview_orbit.cpp`
- Modify: `mergeholo.pro` (register the new reusable header only)

**Interfaces:**
- Produces: `std::vector<osg::Matrixd> buildMultiviewOrbitMatrices(const osg::Vec3d& eye, const osg::Vec3d& viewCenter, const osg::Vec3d& cameraUp, const MultiviewRenderPlan& plan)`.
- Consumes: `MultiviewRenderPlan` and `multiviewOrbitAngles`.
- Guarantees: each returned matrix has `viewCenter` as its look-at target and preserves the original eye-to-target distance.

- [ ] **Step 1: Write the failing test**

Create `vendor/multiview/tests/test_multiview_orbit.cpp` with the following test program.  It intentionally separates the mesh center `(0,0,0)` from the UI-derived target `(0,0,0.10)`.

```cpp
#include "multiviewOrbitMatrices.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "check failed: " << message << '\n';
        std::exit(1);
    }
}

bool sameVector(const osg::Vec3d& left, const osg::Vec3d& right)
{
    return (left - right).length() < 0.000001;
}
}

int main()
{
    const osg::Vec3d eye(0.0, 0.0, 2.0);
    const osg::Vec3d offsetTarget(0.0, 0.0, 0.10);
    const osg::Vec3d up(0.0, 1.0, 0.0);
    const MultiviewRenderPlan plan(2, 2, 10.0);

    const std::vector<osg::Matrixd> matrices = buildMultiviewOrbitMatrices(
        eye, offsetTarget, up, plan);
    expect(matrices.size() == 4, "one matrix is required per multiview frame");

    for (const osg::Matrixd& matrix : matrices) {
        osg::Vec3d frameEye;
        osg::Vec3d frameCenter;
        osg::Vec3d frameUp;
        matrix.getLookAt(frameEye, frameCenter, frameUp);
        expect(sameVector(frameCenter, offsetTarget),
            "every frame must keep the configured offset target");
        expect(std::fabs((frameEye - frameCenter).length() - 1.9) < 0.000001,
            "every frame must preserve its distance from the offset target");
    }
    return 0;
}
```

Create `vendor/multiview/tests/multiview_orbit_tests.pro`:

```qmake
QT += core
QT -= gui widgets
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = multiview_orbit_tests

INCLUDEPATH += ../../base ..
SOURCES += test_multiview_orbit.cpp ../multiviewRenderPlan.cpp
HEADERS += ../multiviewCameraOrbit.h ../multiviewOrbitMatrices.h ../multiviewRenderPlan.h

include(../../../Pri/holo_pipeline.pri)
```

- [ ] **Step 2: Run the test to verify it fails**

Run:

```powershell
qmake vendor/multiview/tests/multiview_orbit_tests.pro -o vendor/multiview/tests/Makefile
nmake /f vendor/multiview/tests/Makefile.Release
vendor/multiview/tests/release/multiview_orbit_tests.exe
```

Expected: compilation fails because `multiviewOrbitMatrices.h` and `buildMultiviewOrbitMatrices` do not exist.

- [ ] **Step 3: Write the minimal implementation**

Create `vendor/multiview/multiviewOrbitMatrices.h`.  Move only the shared, pure camera-basis logic into this header and leave rendering ownership in the two existing renderer classes.

```cpp
#pragma once

#include "multiviewCameraOrbit.h"
#include "multiviewRenderPlan.h"

#include <cmath>
#include <stdexcept>
#include <vector>

inline std::vector<osg::Matrixd> buildMultiviewOrbitMatrices(
    const osg::Vec3d& eye,
    const osg::Vec3d& viewCenter,
    osg::Vec3d up,
    const MultiviewRenderPlan& plan)
{
    osg::Vec3d eyeDirection = eye - viewCenter;
    const double distance = eyeDirection.length();
    if (distance <= 0.000001 || up.normalize() <= 0.000001) {
        throw std::runtime_error("invalid multiview camera basis");
    }
    eyeDirection /= distance;
    osg::Vec3d right = up ^ eyeDirection;
    if (right.normalize() <= 0.000001) {
        throw std::runtime_error("multiview camera up is parallel to its eye direction");
    }
    up = eyeDirection ^ right;
    up.normalize();

    std::vector<osg::Matrixd> matrices;
    matrices.reserve(static_cast<std::size_t>(plan.frameCount()));
    for (int row = 0; row < plan.samplesPerAxis(); ++row) {
        for (int column = 0; column < plan.samplesPerAxis(); ++column) {
            const MultiviewOrbitAngles angles = multiviewOrbitAngles(
                plan.angle(), plan.samplesPerAxis(), plan.stepDegrees(), row, column);
            const double yaw = osg::DegreesToRadians(angles.yawDegrees);
            const double pitch = osg::DegreesToRadians(angles.pitchDegrees);
            const osg::Vec3d horizontal = eyeDirection * std::cos(yaw)
                + right * std::sin(yaw);
            const osg::Vec3d offset = horizontal * std::cos(pitch)
                + up * std::sin(pitch);
            matrices.push_back(osg::Matrixd::lookAt(
                viewCenter + offset * distance, viewCenter, up));
        }
    }
    return matrices;
}
```

Add `vendor/multiview/multiviewOrbitMatrices.h` to the `HEADERS +=` list in `mergeholo.pro`.

- [ ] **Step 4: Run the test to verify it passes**

Run the three commands from Step 2. Expected: process exits with code `0` and no `check failed` output.

- [ ] **Step 5: Commit**

```powershell
git add vendor/multiview/multiviewOrbitMatrices.h vendor/multiview/tests/multiview_orbit_tests.pro vendor/multiview/tests/test_multiview_orbit.cpp mergeholo.pro
git commit -m "test: cover multiview offset orbit center"
```

### Task 2: Use the configured target in every renderer

**Files:**
- Modify: `vendor/multiview/multiviewBatchRenderer.cpp:1-180`
- Modify: `vendor/multiview/multiviewAtlasRenderer.cpp:1-252`
- Test: `vendor/multiview/tests/test_multiview_orbit.cpp`

**Interfaces:**
- Consumes: `buildMultiviewOrbitMatrices(eye, viewCenter, up, plan)` from Task 1.
- Produces: batch and atlas frame views whose `lookAt` target equals the target set by `modelMoveHandler` from `ModelMoveCameraConfig::centerOffset`.

- [ ] **Step 1: Extend the failing regression test**

Add this second block to `main()` after the first loop in `test_multiview_orbit.cpp`:

```cpp
const osg::Vec3d rawMeshCenter(0.0, 0.0, 0.0);
expect(!sameVector(offsetTarget, rawMeshCenter),
    "the fixture requires an offset distinct from the mesh center");
for (const osg::Matrixd& matrix : matrices) {
    osg::Vec3d frameEye;
    osg::Vec3d frameCenter;
    osg::Vec3d frameUp;
    matrix.getLookAt(frameEye, frameCenter, frameUp);
    expect(!sameVector(frameCenter, rawMeshCenter),
        "the renderer must not replace the configured target with the mesh center");
}
```

- [ ] **Step 2: Run the test to verify the unchanged renderers are still not wired to this helper**

Run the commands from Task 1, Step 2. Expected: the helper test passes, but source inspection still finds both legacy assignments:

```powershell
rg -n "const osg::Vec3d orbitCenter = modelTransform_->getBound\(\).center\(\)" vendor/multiview/multiviewBatchRenderer.cpp vendor/multiview/multiviewAtlasRenderer.cpp
```

Expected: two matches, proving the production renderers still ignore the current look-at target before this task's change.

- [ ] **Step 3: Replace each renderer's local orbit calculation**

In `multiviewBatchRenderer.cpp`, include `multiviewOrbitMatrices.h`. After `getViewMatrixAsLookAt(eye, viewCenter, up)`, replace the `orbitCenter`, `eyeDirection`, `distance`, and `right` calculation with:

```cpp
const std::vector<osg::Matrixd> frameViewMatrices =
    buildMultiviewOrbitMatrices(eye, viewCenter, up, plan_);
```

Replace the nested-loop `setViewMatrix(orbitViewMatrix(...))` expression with:

```cpp
viewer_->getCamera()->setViewMatrix(frameViewMatrices.at(
    static_cast<std::size_t>(frameIndex)));
```

In `multiviewAtlasRenderer.cpp`, include `multiviewOrbitMatrices.h`. Replace the entire body of `buildFrameViewMatrices()` after its `getViewMatrixAsLookAt` call with:

```cpp
frameViewMatrices_ = buildMultiviewOrbitMatrices(
    eye, viewCenter, up, renderPlan_);
```

Remove the duplicate local `orbitViewMatrix` functions from both `.cpp` files because the helper owns the one supported orbit calculation.

- [ ] **Step 4: Run focused verification**

Run:

```powershell
qmake vendor/multiview/tests/multiview_orbit_tests.pro -o vendor/multiview/tests/Makefile
nmake /f vendor/multiview/tests/Makefile.Release
vendor/multiview/tests/release/multiview_orbit_tests.exe
rg -n "getBound\(\).center\(\)" vendor/multiview/multiviewBatchRenderer.cpp vendor/multiview/multiviewAtlasRenderer.cpp
```

Expected: the test executable exits `0`; the final `rg` produces no results, because neither generated-view path may replace the UI target with a mesh-bound center.

- [ ] **Step 5: Build the application and commit**

Run:

```powershell
qmake mergeholo.pro -o Makefile.Release
nmake /f Makefile.Release
git add vendor/multiview/multiviewBatchRenderer.cpp vendor/multiview/multiviewAtlasRenderer.cpp vendor/multiview/tests/test_multiview_orbit.cpp
git commit -m "fix: apply UI orbit center to every multiview frame"
```

Expected: `mergeholo.exe` builds successfully and the commit includes only the files in this task.

### Task 3: Perform the UI-driven forward-pivot trial

**Files:**
- Modify: no source files.
- Inspect: `config/default_pipeline.ini` or the runtime pipeline INI emitted by the UI.

**Interfaces:**
- Consumes: UI “远近位置” saved as `multiview_camera_center_offset_z`.
- Produces: generated multiview images orbiting around the configured target.

- [ ] **Step 1: Set the trial value**

In the processing-settings UI, set “远近位置” to `0.10`, select “应用”, and start a new processing run. Do not change “人物大小”.

- [ ] **Step 2: Verify the runtime setting**

Open the run's emitted pipeline INI and check it contains:

```ini
multiview_camera_center_offset_z=0.1
```

- [ ] **Step 3: Compare the result**

Compare the same model generated at `0.00` and `0.10`. Expected: face rotation occurs around a point approximately 0.10 model units in front of the prior mesh-bound center; image framing changes, but no filtering or geometry is altered.

## Plan self-review

- Spec coverage: Task 1 establishes a deterministic nonzero-target test; Task 2 routes that target through batch and atlas paths; Task 3 verifies the existing UI persistence and the requested `+0.10` trial.
- Placeholder scan: no placeholders, deferred steps, or unspecified APIs remain.
- Type consistency: the helper's `MultiviewRenderPlan` input and `std::vector<osg::Matrixd>` output are used consistently by both renderers and the test.

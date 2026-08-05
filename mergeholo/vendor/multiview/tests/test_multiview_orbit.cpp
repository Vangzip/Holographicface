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

} // namespace

int main()
{
    const osg::Vec3d eye(0.0, 0.0, 2.0);
    const osg::Vec3d offsetTarget(0.0, 0.0, 0.10);
    const osg::Vec3d up(0.0, 1.0, 0.0);
    const MultiviewRenderPlan plan(1, 2, 10);

    const std::vector<osg::Matrixd> matrices = buildMultiviewOrbitMatrices(
        eye, offsetTarget, up, plan);
    expect(matrices.size() == 4, "one matrix is required per multiview frame");

    const osg::Vec3d rawMeshCenter(0.0, 0.0, 0.0);
    expect(!sameVector(offsetTarget, rawMeshCenter),
        "the fixture requires an offset distinct from the mesh center");
    bool rawMeshCenterLeavesViewAxis = false;
    for (const osg::Matrixd& matrix : matrices) {
        const osg::Vec3d targetInCamera = matrix.preMult(offsetTarget);
        expect(std::fabs(targetInCamera.x()) < 0.000001
                && std::fabs(targetInCamera.y()) < 0.000001,
            "every frame must keep the configured offset target on its view axis");

        const osg::Vec3d rawCenterInCamera = matrix.preMult(rawMeshCenter);
        rawMeshCenterLeavesViewAxis = rawMeshCenterLeavesViewAxis
            || std::fabs(rawCenterInCamera.x()) >= 0.000001
            || std::fabs(rawCenterInCamera.y()) >= 0.000001;
    }
    expect(rawMeshCenterLeavesViewAxis,
        "the renderer must not replace the configured target with the mesh center");
    return 0;
}

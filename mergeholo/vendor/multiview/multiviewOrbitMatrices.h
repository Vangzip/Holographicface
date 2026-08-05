#pragma once

#include "multiviewCameraOrbit.h"
#include "multiviewRenderPlan.h"

#include <osg/Matrix>

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

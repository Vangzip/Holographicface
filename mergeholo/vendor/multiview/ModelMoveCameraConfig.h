#pragma once

#include "base.h"

struct ModelMoveCameraConfig
{
    double distanceScale = 2.0;
    osg::Vec3d centerOffset = osg::Vec3d(0.0, 0.0, 0.0);
    osg::Vec3d eyeDirection = osg::Vec3d(0.0, -1.0, 0.0);
    osg::Vec3d upDirection = osg::Vec3d(0.0, 0.0, 1.0);
    double fovyDeg = 0.0;
    double zNear = 0.0;
    double zFar = 0.0;
    bool hasInitialRotateXDeg = false;
    bool hasInitialRotateZDeg = false;
    double initialRotateXDeg = 0.0;
    double initialRotateZDeg = 0.0;
    bool captureFlipVertical = true;
};

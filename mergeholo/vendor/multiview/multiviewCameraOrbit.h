#ifndef MULTIVIEW_CAMERA_ORBIT_H
#define MULTIVIEW_CAMERA_ORBIT_H

#include <stdexcept>

struct MultiviewOrbitAngles {
    double pitchDegrees;
    double yawDegrees;
};

constexpr double kMultiviewVerticalAngleScale = 0.5;

inline MultiviewOrbitAngles multiviewOrbitAngles(int angle,
                                                  int samplesPerAxis,
                                                  double stepDegrees,
                                                  int row,
                                                  int column)
{
    if (angle <= 0 || samplesPerAxis <= 0 || stepDegrees <= 0.0
        || row < 0 || row >= samplesPerAxis
        || column < 0 || column >= samplesPerAxis) {
        throw std::out_of_range("invalid multiview orbit sample");
    }

    const double halfAngle = static_cast<double>(angle) / 2.0;
    const double verticalStep = stepDegrees * kMultiviewVerticalAngleScale;
    const double firstYaw = -halfAngle + stepDegrees / 2.0;
    const double firstPitch = halfAngle * kMultiviewVerticalAngleScale
        - verticalStep / 2.0;
    return {
        firstPitch - static_cast<double>(row) * verticalStep,
        firstYaw + static_cast<double>(column) * stepDegrees
    };
}

#endif

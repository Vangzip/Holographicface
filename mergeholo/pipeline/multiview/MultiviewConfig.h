#pragma once

#include "ModelMoveCameraConfig.h"

#include <string>

struct MultiviewConfig {
    std::string modelType = "obj";
    int angle = 90;
    int per = 3;
    int resolution = 150;
    int atlasSize = 4096;
    ModelMoveCameraConfig camera;
};

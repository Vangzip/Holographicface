#include "multiviewGraphicsConfig.h"

MultiviewGraphicsConfig makeMultiviewGraphicsConfig(bool memoryMode) {
    MultiviewGraphicsConfig config;
    config.windowDecoration = !memoryMode;
    config.doubleBuffer = !memoryMode;
    config.pbuffer = memoryMode;
    config.vsync = false;
    config.drawBuffer = memoryMode ? MultiviewDrawBufferFront : MultiviewDrawBufferBack;
    config.readBuffer = memoryMode ? MultiviewDrawBufferFront : MultiviewDrawBufferBack;
    return config;
}

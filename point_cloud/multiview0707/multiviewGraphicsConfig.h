#ifndef MULTIVIEW_GRAPHICS_CONFIG_H
#define MULTIVIEW_GRAPHICS_CONFIG_H

enum MultiviewDrawBuffer {
    MultiviewDrawBufferFront,
    MultiviewDrawBufferBack
};

struct MultiviewGraphicsConfig {
    bool windowDecoration;
    bool doubleBuffer;
    bool pbuffer;
    bool vsync;
    MultiviewDrawBuffer drawBuffer;
    MultiviewDrawBuffer readBuffer;
};

MultiviewGraphicsConfig makeMultiviewGraphicsConfig(bool memoryMode);

#endif

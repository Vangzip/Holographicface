#pragma once

struct ElementalConfig {
    int targetRows = 150;
    int targetCols = 150;
    int viewRows = 270;
    int viewCols = 270;
    int viewNameDigits = 3;
    int jpgQuality = 100;
    int writerThreads = 0; // 0 = auto, capped for memory-bandwidth-bound transform
    bool flipSourceY = true;
    bool flipViewRows = true;
};

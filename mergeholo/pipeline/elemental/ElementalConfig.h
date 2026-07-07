#pragma once

struct ElementalConfig {
    int targetRows = 150;
    int targetCols = 150;
    int viewRows = 270;
    int viewCols = 270;
    int viewNameDigits = 3;
    int jpgQuality = 100;
    int writerThreads = 1;
    bool flipSourceY = true;
    bool flipViewRows = true;
};

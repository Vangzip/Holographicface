#pragma once

#include <cstddef>

struct ElementalMemoryTransformConfig {
    int viewRows = 0;
    int viewCols = 0;
    int sourceRows = 0;
    int sourceCols = 0;
    int targetRows = 0;
    int targetCols = 0;
    bool flipSourceY = false;
    bool flipViewRows = false;
    bool sourceRowsBottomUp = false;
    int threadCount = 1;
};

enum class ElementalMemoryTransformStatus {
    Ok,
    InvalidArgument,
    SizeOverflow
};

bool computeElementalMemoryOutputSize(
    int viewRows,
    int viewCols,
    int targetRows,
    int targetCols,
    std::size_t& imageCount,
    std::size_t& imageBytes,
    std::size_t& totalBytes);

int chooseElementalMemoryThreads(int requested, int targetPixels);

ElementalMemoryTransformStatus storeElementalFromMemory(
    const unsigned char* source,
    std::size_t sourceFrameBytes,
    unsigned char* output,
    const ElementalMemoryTransformConfig& config);

ElementalMemoryTransformStatus storeElementalFromMemoryBlocked(
    const unsigned char* source,
    std::size_t sourceFrameBytes,
    unsigned char* output,
    const ElementalMemoryTransformConfig& config);

#pragma once

#include <cstddef>
#include <cstring>
#include <memory>

enum class ElementalMemoryMode {
    None,
    Materialized
};

struct ElementalMemoryResult {
    // Materialized images always use packed RGB byte order.
    std::unique_ptr<unsigned char[]> pixels;
    size_t imageCount = 0;
    size_t imageBytes = 0;
    size_t totalBytes = 0;
    int rows = 0;
    int cols = 0;
    int targetRows = 0;
    int targetCols = 0;
    int sourceRows = 0;
    int sourceCols = 0;
    bool flipSourceY = true;
    bool flipViewRows = true;
    bool sourceRowsBottomUp = true;
    ElementalMemoryMode mode = ElementalMemoryMode::None;

    void clear()
    {
        pixels.reset();
        imageCount = 0;
        imageBytes = 0;
        totalBytes = 0;
        rows = 0;
        cols = 0;
        targetRows = 0;
        targetCols = 0;
        sourceRows = 0;
        sourceCols = 0;
        flipSourceY = true;
        flipViewRows = true;
        sourceRowsBottomUp = true;
        mode = ElementalMemoryMode::None;
    }

    bool hasResult() const
    {
        return isMaterialized();
    }

    bool isMaterialized() const
    {
        return mode == ElementalMemoryMode::Materialized && pixels;
    }

    bool copyImage(size_t targetIndex, unsigned char* destination) const
    {
        if (destination == nullptr || targetIndex >= imageCount || imageBytes == 0) {
            return false;
        }

        if (isMaterialized()) {
            std::memcpy(destination, pixels.get() + targetIndex * imageBytes, imageBytes);
            return true;
        }
        return false;
    }
};

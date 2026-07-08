#pragma once

#include "memoryFrameSink.h"

#include <cstddef>
#include <cstring>
#include <memory>
#include <new>

enum class ElementalMemoryMode {
    None,
    Materialized,
    VirtualFromMultiview
};

struct ElementalMemoryResult {
    std::unique_ptr<unsigned char[]> pixels;
    std::shared_ptr<const MemoryFrameSink> sourceSink;
    size_t imageCount = 0;
    size_t imageBytes = 0;
    size_t totalBytes = 0;
    int rows = 0;
    int cols = 0;
    int targetRows = 0;
    int targetCols = 0;
    int sourceRows = 0;
    int sourceCols = 0;
    size_t sourceFrameBytes = 0;
    bool flipSourceY = true;
    bool flipViewRows = true;
    bool sourceRowsBottomUp = true;
    ElementalMemoryMode mode = ElementalMemoryMode::None;

    void clear()
    {
        pixels.reset();
        sourceSink.reset();
        imageCount = 0;
        imageBytes = 0;
        totalBytes = 0;
        rows = 0;
        cols = 0;
        targetRows = 0;
        targetCols = 0;
        sourceRows = 0;
        sourceCols = 0;
        sourceFrameBytes = 0;
        flipSourceY = true;
        flipViewRows = true;
        sourceRowsBottomUp = true;
        mode = ElementalMemoryMode::None;
    }

    bool hasResult() const
    {
        return mode == ElementalMemoryMode::Materialized
            ? static_cast<bool>(pixels)
            : mode == ElementalMemoryMode::VirtualFromMultiview && sourceSink;
    }

    bool isMaterialized() const
    {
        return mode == ElementalMemoryMode::Materialized && pixels;
    }

    bool isVirtual() const
    {
        return mode == ElementalMemoryMode::VirtualFromMultiview && sourceSink;
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

        if (!isVirtual() || rows <= 0 || cols <= 0 || targetCols <= 0
            || sourceRows <= 0 || sourceCols <= 0 || sourceFrameBytes == 0) {
            return false;
        }

        const int targetRow = static_cast<int>(targetIndex / static_cast<size_t>(targetCols));
        const int targetCol = static_cast<int>(targetIndex % static_cast<size_t>(targetCols));
        const int logicalSourceRow = flipSourceY
            ? sourceRows - 1 - targetRow
            : targetRow;
        const int sourceTargetRow = sourceRowsBottomUp
            ? sourceRows - 1 - logicalSourceRow
            : logicalSourceRow;
        const bool targetPixelInSource = targetCol >= 0 && targetCol < sourceCols
            && sourceTargetRow >= 0 && sourceTargetRow < sourceRows;

        if (!targetPixelInSource) {
            std::memset(destination, 0, imageBytes);
            return true;
        }

        const unsigned char* source = sourceSink->data();
        for (int outputViewRow = 0; outputViewRow < rows; ++outputViewRow) {
            const int sourceViewRow = flipViewRows
                ? rows - 1 - outputViewRow
                : outputViewRow;
            const size_t sourceViewRowOffset = static_cast<size_t>(sourceViewRow)
                * static_cast<size_t>(cols);
            unsigned char* dst = destination
                + static_cast<size_t>(outputViewRow) * static_cast<size_t>(cols) * 3;

            for (int outputViewCol = 0; outputViewCol < cols; ++outputViewCol) {
                const size_t viewIndex = sourceViewRowOffset + static_cast<size_t>(outputViewCol);
                const unsigned char* src = source
                    + viewIndex * sourceFrameBytes
                    + (static_cast<size_t>(sourceTargetRow) * static_cast<size_t>(sourceCols)
                        + static_cast<size_t>(targetCol)) * 3;
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
                dst += 3;
            }
        }
        return true;
    }

    bool materialize()
    {
        if (isMaterialized()) {
            return true;
        }
        if (!isVirtual()) {
            return false;
        }

        std::unique_ptr<unsigned char[]> materialized;
        try {
            materialized.reset(new unsigned char[totalBytes]);
        }
        catch (const std::bad_alloc&) {
            return false;
        }

        for (size_t index = 0; index < imageCount; ++index) {
            if (!copyImage(index, materialized.get() + index * imageBytes)) {
                return false;
            }
        }

        pixels = std::move(materialized);
        sourceSink.reset();
        mode = ElementalMemoryMode::Materialized;
        return true;
    }
};

#include "ElementalMemoryTransform.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <thread>
#include <vector>

namespace {

bool checkedMultiply(std::size_t left, std::size_t right, std::size_t& result) {
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        return false;
    }
    result = left * right;
    return true;
}

bool positiveDimensions(const ElementalMemoryTransformConfig& config) {
    return config.viewRows > 0
        && config.viewCols > 0
        && config.sourceRows > 0
        && config.sourceCols > 0
        && config.targetRows > 0
        && config.targetCols > 0;
}

void storeTargetRange(
    const unsigned char* source,
    std::size_t sourceFrameBytes,
    unsigned char* output,
    const ElementalMemoryTransformConfig& config,
    int beginTarget,
    int endTarget) {
    const std::size_t outputImageBytes = static_cast<std::size_t>(config.viewRows)
        * static_cast<std::size_t>(config.viewCols) * 3;
    const bool fullCoverage = config.sourceRows == config.targetRows
        && config.sourceCols == config.targetCols;

    for (int targetIndex = beginTarget; targetIndex < endTarget; ++targetIndex) {
        const int targetRow = targetIndex / config.targetCols;
        const int targetCol = targetIndex % config.targetCols;
        unsigned char* outputBase = output + static_cast<std::size_t>(targetIndex) * outputImageBytes;

        const int logicalSourceRow = config.flipSourceY
            ? config.sourceRows - 1 - targetRow
            : targetRow;
        const int sourceTargetRow = config.sourceRowsBottomUp
            ? config.sourceRows - 1 - logicalSourceRow
            : logicalSourceRow;
        const bool targetPixelInSource = targetCol >= 0 && targetCol < config.sourceCols
            && sourceTargetRow >= 0 && sourceTargetRow < config.sourceRows;

        if (!fullCoverage || !targetPixelInSource) {
            std::memset(outputBase, 0, outputImageBytes);
        }
        if (!targetPixelInSource) {
            continue;
        }

        for (int outputViewRow = 0; outputViewRow < config.viewRows; ++outputViewRow) {
            const int sourceViewRow = config.flipViewRows
                ? config.viewRows - 1 - outputViewRow
                : outputViewRow;
            const std::size_t sourceViewRowOffset = static_cast<std::size_t>(sourceViewRow)
                * static_cast<std::size_t>(config.viewCols);
            unsigned char* dst = outputBase
                + static_cast<std::size_t>(outputViewRow) * static_cast<std::size_t>(config.viewCols) * 3;

            for (int outputViewCol = 0; outputViewCol < config.viewCols; ++outputViewCol) {
                const std::size_t viewIndex = sourceViewRowOffset + static_cast<std::size_t>(outputViewCol);
                const unsigned char* src = source
                    + viewIndex * sourceFrameBytes
                    + (static_cast<std::size_t>(sourceTargetRow) * static_cast<std::size_t>(config.sourceCols)
                        + static_cast<std::size_t>(targetCol)) * 3;
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
                dst += 3;
            }
        }
    }
}

} // namespace

bool computeElementalMemoryOutputSize(
    int viewRows,
    int viewCols,
    int targetRows,
    int targetCols,
    std::size_t& imageCount,
    std::size_t& imageBytes,
    std::size_t& totalBytes) {
    imageCount = 0;
    imageBytes = 0;
    totalBytes = 0;
    if (viewRows <= 0 || viewCols <= 0 || targetRows <= 0 || targetCols <= 0) {
        return false;
    }

    std::size_t viewCount = 0;
    if (!checkedMultiply(static_cast<std::size_t>(viewRows), static_cast<std::size_t>(viewCols), viewCount)
        || !checkedMultiply(static_cast<std::size_t>(targetRows), static_cast<std::size_t>(targetCols), imageCount)
        || !checkedMultiply(viewCount, static_cast<std::size_t>(3), imageBytes)
        || !checkedMultiply(imageCount, imageBytes, totalBytes)) {
        imageCount = 0;
        imageBytes = 0;
        totalBytes = 0;
        return false;
    }
    return true;
}

int chooseElementalMemoryThreads(int requested, int targetPixels) {
    if (targetPixels <= 1) {
        return 1;
    }
    if (requested > 0) {
        return std::max(1, std::min(requested, targetPixels));
    }
    const unsigned int hardware = std::thread::hardware_concurrency();
    const int fallback = hardware == 0 ? 4 : static_cast<int>(hardware);
    return std::max(1, std::min(fallback, targetPixels));
}

ElementalMemoryTransformStatus storeElementalFromMemory(
    const unsigned char* source,
    std::size_t sourceFrameBytes,
    unsigned char* output,
    const ElementalMemoryTransformConfig& config) {
    if (source == nullptr || output == nullptr || !positiveDimensions(config)) {
        return ElementalMemoryTransformStatus::InvalidArgument;
    }

    std::size_t minimumSourceFrameBytes = 0;
    if (!checkedMultiply(static_cast<std::size_t>(config.sourceRows), static_cast<std::size_t>(config.sourceCols), minimumSourceFrameBytes)
        || !checkedMultiply(minimumSourceFrameBytes, static_cast<std::size_t>(3), minimumSourceFrameBytes)) {
        return ElementalMemoryTransformStatus::SizeOverflow;
    }
    if (sourceFrameBytes < minimumSourceFrameBytes) {
        return ElementalMemoryTransformStatus::InvalidArgument;
    }

    std::size_t imageCount = 0;
    std::size_t imageBytes = 0;
    std::size_t totalBytes = 0;
    if (!computeElementalMemoryOutputSize(
            config.viewRows, config.viewCols, config.targetRows, config.targetCols,
            imageCount, imageBytes, totalBytes)) {
        return ElementalMemoryTransformStatus::SizeOverflow;
    }
    if (imageCount > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return ElementalMemoryTransformStatus::SizeOverflow;
    }

    const int targetPixels = static_cast<int>(imageCount);
    const int threadCount = chooseElementalMemoryThreads(config.threadCount, targetPixels);
    if (threadCount == 1) {
        storeTargetRange(source, sourceFrameBytes, output, config, 0, targetPixels);
        return ElementalMemoryTransformStatus::Ok;
    }

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threadCount));
    const int chunk = (targetPixels + threadCount - 1) / threadCount;
    for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
        const int begin = threadIndex * chunk;
        const int end = std::min(targetPixels, begin + chunk);
        if (begin >= end) {
            continue;
        }
        try {
            workers.emplace_back([=] {
                storeTargetRange(source, sourceFrameBytes, output, config, begin, end);
            });
        }
        catch (...) {
            for (std::thread& worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
            storeTargetRange(source, sourceFrameBytes, output, config, 0, targetPixels);
            return ElementalMemoryTransformStatus::Ok;
        }
    }
    for (std::thread& worker : workers) {
        worker.join();
    }

    return ElementalMemoryTransformStatus::Ok;
}

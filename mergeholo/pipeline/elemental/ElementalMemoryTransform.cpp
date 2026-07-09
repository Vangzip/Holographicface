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

bool validateTransformArguments(
    const unsigned char* source,
    std::size_t sourceFrameBytes,
    unsigned char* output,
    const ElementalMemoryTransformConfig& config,
    std::size_t& imageCount,
    std::size_t& imageBytes,
    std::size_t& totalBytes,
    std::size_t& minimumSourceFrameBytes) {
    imageCount = 0;
    imageBytes = 0;
    totalBytes = 0;
    minimumSourceFrameBytes = 0;

    if (source == nullptr || output == nullptr || !positiveDimensions(config)) {
        return false;
    }

    if (!checkedMultiply(static_cast<std::size_t>(config.sourceRows), static_cast<std::size_t>(config.sourceCols), minimumSourceFrameBytes)
        || !checkedMultiply(minimumSourceFrameBytes, static_cast<std::size_t>(3), minimumSourceFrameBytes)) {
        return false;
    }
    if (sourceFrameBytes < minimumSourceFrameBytes) {
        return false;
    }

    if (!computeElementalMemoryOutputSize(
            config.viewRows, config.viewCols, config.targetRows, config.targetCols,
            imageCount, imageBytes, totalBytes)) {
        return false;
    }
    if (imageCount > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    return true;
}

void storeTargetRangeScalar(
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

ElementalMemoryTransformStatus storeElementalFromMemoryScalar(
    const unsigned char* source,
    std::size_t sourceFrameBytes,
    unsigned char* output,
    const ElementalMemoryTransformConfig& config) {
    std::size_t imageCount = 0;
    std::size_t imageBytes = 0;
    std::size_t totalBytes = 0;
    std::size_t minimumSourceFrameBytes = 0;
    if (!validateTransformArguments(source, sourceFrameBytes, output, config,
            imageCount, imageBytes, totalBytes, minimumSourceFrameBytes)) {
        return (source == nullptr || output == nullptr || !positiveDimensions(config) || sourceFrameBytes < minimumSourceFrameBytes)
            ? ElementalMemoryTransformStatus::InvalidArgument
            : ElementalMemoryTransformStatus::SizeOverflow;
    }

    const int targetPixels = static_cast<int>(imageCount);
    const int threadCount = chooseElementalMemoryThreads(config.threadCount, targetPixels);
    if (threadCount == 1) {
        storeTargetRangeScalar(source, sourceFrameBytes, output, config, 0, targetPixels);
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
                storeTargetRangeScalar(source, sourceFrameBytes, output, config, begin, end);
            });
        }
        catch (...) {
            for (std::thread& worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
            storeTargetRangeScalar(source, sourceFrameBytes, output, config, 0, targetPixels);
            return ElementalMemoryTransformStatus::Ok;
        }
    }
    for (std::thread& worker : workers) {
        worker.join();
    }

    return ElementalMemoryTransformStatus::Ok;
}

int sourceRowForTargetRow(const ElementalMemoryTransformConfig& config, int targetRow) {
    const int logicalSourceRow = config.flipSourceY
        ? config.sourceRows - 1 - targetRow
        : targetRow;
    return config.sourceRowsBottomUp
        ? config.sourceRows - 1 - logicalSourceRow
        : logicalSourceRow;
}

int sourceViewForOutputView(const ElementalMemoryTransformConfig& config, int outputViewIndex) {
    const int outputViewRow = outputViewIndex / config.viewCols;
    const int outputViewCol = outputViewIndex - outputViewRow * config.viewCols;
    const int sourceViewRow = config.flipViewRows
        ? config.viewRows - 1 - outputViewRow
        : outputViewRow;
    return sourceViewRow * config.viewCols + outputViewCol;
}

void storeTargetRowsBlockedFullCoverage(
    const unsigned char* source,
    std::size_t sourceFrameBytes,
    unsigned char* output,
    const ElementalMemoryTransformConfig& config,
    const std::vector<int>& sourceRows,
    const std::vector<int>& sourceViews,
    int beginTargetRow,
    int endTargetRow) {
    // 16 target pixels x 512 views x RGB = 24 KiB, usually inside L1 cache.
    constexpr int kTargetBlockCols = 16;
    constexpr int kViewBlock = 512;

    const int viewCount = config.viewRows * config.viewCols;
    const std::size_t outputImageBytes = static_cast<std::size_t>(viewCount) * 3;
    std::vector<unsigned char> tile(static_cast<std::size_t>(kTargetBlockCols) * kViewBlock * 3);

    for (int targetRow = beginTargetRow; targetRow < endTargetRow; ++targetRow) {
        const int srcRow = sourceRows[static_cast<std::size_t>(targetRow)];
        const std::size_t srcRowOffset = static_cast<std::size_t>(srcRow)
            * static_cast<std::size_t>(config.sourceCols) * 3;
        const std::size_t targetRowBase = static_cast<std::size_t>(targetRow)
            * static_cast<std::size_t>(config.targetCols);

        for (int targetCol = 0; targetCol < config.targetCols; targetCol += kTargetBlockCols) {
            const int targetBlock = std::min(kTargetBlockCols, config.targetCols - targetCol);
            const std::size_t sourcePixelOffset = srcRowOffset + static_cast<std::size_t>(targetCol) * 3;

            for (int viewBlockBegin = 0; viewBlockBegin < viewCount; viewBlockBegin += kViewBlock) {
                const int viewBlock = std::min(kViewBlock, viewCount - viewBlockBegin);
                const std::size_t tileViewStride = static_cast<std::size_t>(viewBlock) * 3;

                // Load a small [target x view] transposed tile. Each source read is a contiguous RGB run.
                for (int localView = 0; localView < viewBlock; ++localView) {
                    const int sourceView = sourceViews[static_cast<std::size_t>(viewBlockBegin + localView)];
                    const unsigned char* src = source
                        + static_cast<std::size_t>(sourceView) * sourceFrameBytes
                        + sourcePixelOffset;
                    unsigned char* dst = tile.data() + static_cast<std::size_t>(localView) * 3;

                    for (int localTarget = 0; localTarget < targetBlock; ++localTarget) {
                        dst[0] = src[0];
                        dst[1] = src[1];
                        dst[2] = src[2];
                        src += 3;
                        dst += tileViewStride;
                    }
                }

                // Store each output image segment as one contiguous write.
                const std::size_t outputViewOffset = static_cast<std::size_t>(viewBlockBegin) * 3;
                const std::size_t copyBytes = static_cast<std::size_t>(viewBlock) * 3;
                for (int localTarget = 0; localTarget < targetBlock; ++localTarget) {
                    const std::size_t targetIndex = targetRowBase
                        + static_cast<std::size_t>(targetCol + localTarget);
                    std::memcpy(
                        output + targetIndex * outputImageBytes + outputViewOffset,
                        tile.data() + static_cast<std::size_t>(localTarget) * tileViewStride,
                        copyBytes);
                }
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

int chooseElementalMemoryThreads(int requested, int workItems) {
    if (workItems <= 1) {
        return 1;
    }
    if (requested > 0) {
        return std::max(1, std::min(requested, workItems));
    }

    const unsigned int hardware = std::thread::hardware_concurrency();
    const int fallback = hardware == 0 ? 4 : static_cast<int>(hardware);
    // This transform is memory-bandwidth bound; using all logical CPUs often slows it down.
    const int bandwidthFriendlyCap = 16;
    return std::max(1, std::min({ fallback, bandwidthFriendlyCap, workItems }));
}

ElementalMemoryTransformStatus storeElementalFromMemoryBlocked(
    const unsigned char* source,
    std::size_t sourceFrameBytes,
    unsigned char* output,
    const ElementalMemoryTransformConfig& config) {
    std::size_t imageCount = 0;
    std::size_t imageBytes = 0;
    std::size_t totalBytes = 0;
    std::size_t minimumSourceFrameBytes = 0;
    if (!validateTransformArguments(source, sourceFrameBytes, output, config,
            imageCount, imageBytes, totalBytes, minimumSourceFrameBytes)) {
        return (source == nullptr || output == nullptr || !positiveDimensions(config) || sourceFrameBytes < minimumSourceFrameBytes)
            ? ElementalMemoryTransformStatus::InvalidArgument
            : ElementalMemoryTransformStatus::SizeOverflow;
    }

    const bool fullCoverage = config.sourceRows == config.targetRows
        && config.sourceCols == config.targetCols;
    if (!fullCoverage) {
        // Keep the edge-safe scalar path for mismatched image/target grids.
        return storeElementalFromMemoryScalar(source, sourceFrameBytes, output, config);
    }

    const int viewCount = config.viewRows * config.viewCols;
    std::vector<int> sourceRows(static_cast<std::size_t>(config.targetRows));
    for (int targetRow = 0; targetRow < config.targetRows; ++targetRow) {
        sourceRows[static_cast<std::size_t>(targetRow)] = sourceRowForTargetRow(config, targetRow);
    }

    std::vector<int> sourceViews(static_cast<std::size_t>(viewCount));
    for (int outputView = 0; outputView < viewCount; ++outputView) {
        sourceViews[static_cast<std::size_t>(outputView)] = sourceViewForOutputView(config, outputView);
    }

    const int threadCount = chooseElementalMemoryThreads(config.threadCount, config.targetRows);
    if (threadCount == 1) {
        storeTargetRowsBlockedFullCoverage(
            source, sourceFrameBytes, output, config, sourceRows, sourceViews,
            0, config.targetRows);
        return ElementalMemoryTransformStatus::Ok;
    }

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threadCount));
    const int rowsPerThread = (config.targetRows + threadCount - 1) / threadCount;

    try {
        for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
            const int beginRow = threadIndex * rowsPerThread;
            const int endRow = std::min(config.targetRows, beginRow + rowsPerThread);
            if (beginRow >= endRow) {
                continue;
            }
            workers.emplace_back([&, beginRow, endRow] {
                storeTargetRowsBlockedFullCoverage(
                    source, sourceFrameBytes, output, config, sourceRows, sourceViews,
                    beginRow, endRow);
            });
        }
    }
    catch (...) {
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        return storeElementalFromMemoryScalar(source, sourceFrameBytes, output, config);
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    return ElementalMemoryTransformStatus::Ok;
}

ElementalMemoryTransformStatus storeElementalFromMemory(
    const unsigned char* source,
    std::size_t sourceFrameBytes,
    unsigned char* output,
    const ElementalMemoryTransformConfig& config) {
    return storeElementalFromMemoryBlocked(source, sourceFrameBytes, output, config);
}

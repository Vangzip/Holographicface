#pragma once

#include "memoryFrameSink.h"
#include "multiviewRenderPlan.h"

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>

struct MultiviewMemoryResult {
    std::shared_ptr<MemoryFrameSink> sink;
    std::shared_ptr<MultiviewRenderPlan> plan;
    bool directAtlasElemental = false;
    std::uint64_t directFramesCaptured = 0;
    std::uint64_t directBytesCaptured = 0;
    std::uint64_t directPagesRendered = 0;
    std::uint64_t directPageReadbacks = 0;
    unsigned int directReadbackErrors = 0;
    double directRenderSeconds = 0.0;
    double directReadbackSeconds = 0.0;
    double directCopySeconds = 0.0;
    double directTotalSeconds = 0.0;
    double directScatterSeconds = 0.0;
    std::string modelSource;
    double modelBuildSeconds = 0.0;
    std::size_t modelVertexCount = 0;
    std::size_t modelTriangleCount = 0;
    std::size_t modelSkippedFaces = 0;
};

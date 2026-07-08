#pragma once

#include "PipelineContext.h"

#include <vector>

void writePipelineLog(
    const HoloConfig& config,
    const std::vector<StageTiming>& timings,
    const MultiviewMemoryResult& multiviewMemory,
    const ElementalMemoryResult& elementalMemory,
    bool depthToMeshMemory,
    bool meshToModelMemory,
    int resultCode,
    double wallSeconds);

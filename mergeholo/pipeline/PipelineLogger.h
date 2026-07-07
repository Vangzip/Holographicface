#pragma once

#include "PipelineContext.h"

#include <vector>

void writePipelineLog(
    const HoloConfig& config,
    const std::vector<StageTiming>& timings,
    const MultiviewMemoryResult& multiviewMemory,
    const ElementalMemoryResult& elementalMemory,
    int resultCode,
    double wallSeconds);

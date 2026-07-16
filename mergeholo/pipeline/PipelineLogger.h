#pragma once

#include "PipelineContext.h"
#include "ResultSaveSettings.h"

#include <vector>

void writePipelineLog(
    const HoloConfig& config,
    const std::vector<StageTiming>& timings,
    const MultiviewMemoryResult& multiviewMemory,
    const ElementalMemoryResult& elementalMemory,
    const ResultSaveReport& saveReport,
    bool depthToMeshMemory,
    bool meshToModelMemory,
    int resultCode,
    double wallSeconds);

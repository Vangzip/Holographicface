#pragma once

#include "PipelineData.h"

int runElementalStage(
    const HoloConfig& config,
    const CliOptions& options,
    const MultiviewMemoryResult* memoryResult,
    ElementalMemoryResult* elementalResult);

#pragma once

#include "PipelineContext.h"

int runElementalStage(
    const HoloConfig& config,
    const CliOptions& options,
    const MultiviewMemoryResult* memoryResult,
    ElementalMemoryResult* elementalResult);

#pragma once

#include "PipelineContext.h"

int processElemental(
    const HoloConfig& config,
    const CliOptions& options,
    const MultiviewMemoryResult* memoryResult,
    ElementalMemoryResult* elementalResult);

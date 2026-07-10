#pragma once

#include "PipelineContext.h"

struct MeshMemoryResult;

int runMultiviewStage(HoloConfig& config, const CliOptions& options, MultiviewMemoryResult* memoryResult);
int runMultiviewStage(
    HoloConfig& config,
    const CliOptions& options,
    MultiviewMemoryResult* memoryResult,
    ElementalMemoryResult* directElementalResult);
int runMultiviewStage(
    HoloConfig& config,
    const CliOptions& options,
    MultiviewMemoryResult* memoryResult,
    ElementalMemoryResult* directElementalResult,
    const MeshMemoryResult* meshMemory);

#pragma once

#include "DepthMeshModelMemory.h"
#include "PipelineContext.h"

int runMeshOneStage(const HoloConfig& config, const CliOptions& options);
int runMeshStage(const HoloConfig& config, const CliOptions& options);
int runMeshStage(
    const HoloConfig& config,
    const CliOptions& options,
    const DepthMemoryResult* depthMemory,
    MeshMemoryResult* meshMemory);

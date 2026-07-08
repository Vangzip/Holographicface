#pragma once

#include "DepthMeshModelMemory.h"
#include "PipelineContext.h"

int runDepthStage(const HoloConfig& config, const CliOptions& options);
int runDepthStage(const HoloConfig& config, const CliOptions& options, DepthMemoryResult* memoryResult);

#pragma once

#include "DepthMeshModelMemory.h"
#include "PipelineContext.h"

int runModelStage(const HoloConfig& config, const CliOptions& options);
int runModelStage(const HoloConfig& config, const CliOptions& options, const MeshMemoryResult* meshMemory);

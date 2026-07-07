#pragma once

#include "memoryFrameSink.h"
#include "multiviewRenderPlan.h"

#include <memory>

struct MultiviewMemoryResult {
    std::unique_ptr<MemoryFrameSink> sink;
    std::unique_ptr<MultiviewRenderPlan> plan;
};

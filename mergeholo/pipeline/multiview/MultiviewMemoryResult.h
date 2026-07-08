#pragma once

#include "memoryFrameSink.h"
#include "multiviewRenderPlan.h"

#include <memory>

struct MultiviewMemoryResult {
    std::shared_ptr<MemoryFrameSink> sink;
    std::shared_ptr<MultiviewRenderPlan> plan;
};

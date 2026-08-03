#ifndef MULTIVIEW_MEMORY_DUMP_H
#define MULTIVIEW_MEMORY_DUMP_H

#include "memoryFrameSink.h"
#include "multiviewRenderPlan.h"

#include <cstdint>
#include <string>

struct MultiviewMemoryDumpStats {
    std::uint64_t framesWritten;
    std::uint64_t writeErrors;
    double seconds;
};

MultiviewMemoryDumpStats dumpMultiviewMemoryFrames(const MultiviewRenderPlan& plan,
                                                   const MemoryFrameSink& sink,
                                                   const std::string& outputDirectory);

#endif

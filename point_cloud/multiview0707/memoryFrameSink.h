#ifndef MEMORY_FRAME_SINK_H
#define MEMORY_FRAME_SINK_H

#include "multiviewRenderPlan.h"

#include <cstdint>
#include <memory>

class MemoryFrameSink {
public:
    MemoryFrameSink(const MultiviewRenderPlan& plan, bool allocateStorage);

    std::uint64_t frameCount() const;
    std::uint64_t frameBytes() const;
    std::uint64_t totalBytes() const;
    unsigned char* data();
    const unsigned char* data() const;
    unsigned char* frameData(std::uint64_t frameIndex);

private:
    std::uint64_t frameCount_;
    std::uint64_t frameBytes_;
    std::uint64_t totalBytes_;
    std::unique_ptr<unsigned char[]> buffer_;
};

#endif

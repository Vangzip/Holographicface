#ifndef MEMORY_FRAME_SINK_H
#define MEMORY_FRAME_SINK_H

#include "memoryAtlasPageSink.h"
#include "multiviewRenderPlan.h"

#include <cstdint>
#include <memory>

class MemoryFrameSink : public MemoryAtlasPageSink {
public:
    MemoryFrameSink(const MultiviewRenderPlan& plan, bool allocateStorage);
    MemoryFrameSink(const MultiviewAtlasPlan& atlasPlan,
                    const MultiviewRenderPlan& plan,
                    bool allocateStorage);

    std::uint64_t frameCount() const;
    std::uint64_t frameBytes() const;
    std::uint64_t totalBytes() const;
    unsigned char* data();
    const unsigned char* data() const;
    unsigned char* frameData(std::uint64_t frameIndex);
    void afterPageReadback(std::uint64_t pageIndex) override;
    std::uint64_t capturedBytes(std::uint64_t pageReadbacks) const override;

private:
    MultiviewRenderPlan renderPlan_;
    std::uint64_t frameCount_;
    std::uint64_t frameBytes_;
    std::uint64_t totalBytes_;
    std::unique_ptr<unsigned char[]> buffer_;
};

#endif

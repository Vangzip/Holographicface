#include "memoryFrameSink.h"

#include <limits>
#include <new>
#include <stdexcept>

MemoryFrameSink::MemoryFrameSink(const MultiviewRenderPlan& plan, bool allocateStorage)
    : frameCount_(plan.frameCount()),
      frameBytes_(plan.frameBytes()),
      totalBytes_(plan.totalBytes()) {
    if (allocateStorage) {
        if (totalBytes_ > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            throw std::length_error("frame buffer is too large for this platform");
        }
        buffer_.reset(new unsigned char[static_cast<std::size_t>(totalBytes_)]);
    }
}

std::uint64_t MemoryFrameSink::frameCount() const {
    return frameCount_;
}

std::uint64_t MemoryFrameSink::frameBytes() const {
    return frameBytes_;
}

std::uint64_t MemoryFrameSink::totalBytes() const {
    return totalBytes_;
}

unsigned char* MemoryFrameSink::data() {
    return buffer_.get();
}

const unsigned char* MemoryFrameSink::data() const {
    return buffer_.get();
}

unsigned char* MemoryFrameSink::frameData(std::uint64_t frameIndex) {
    if (frameIndex >= frameCount_) {
        throw std::out_of_range("frame index out of range");
    }
    if (!buffer_) {
        throw std::logic_error("frame buffer storage was not allocated");
    }
    return buffer_.get() + static_cast<std::size_t>(frameIndex * frameBytes_);
}

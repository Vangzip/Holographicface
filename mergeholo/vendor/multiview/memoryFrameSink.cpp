#include "memoryFrameSink.h"

#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

MemoryFrameSink::MemoryFrameSink(const MultiviewRenderPlan& plan, bool allocateStorage)
    : MemoryAtlasPageSink(MultiviewAtlasPlan(plan, plan.resolution()), false),
      renderPlan_(plan),
      frameCount_(plan.frameCount()),
      frameBytes_(plan.frameBytes()),
      totalBytes_(plan.totalBytes()) {
    if (allocateStorage) {
        if (totalBytes_ > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            throw std::length_error("frame buffer is too large for this platform");
        }
        buffer_.reset(new unsigned char[static_cast<std::size_t>(totalBytes_)]);
    }
}

MemoryFrameSink::MemoryFrameSink(
    const MultiviewAtlasPlan& atlasPlan,
    const MultiviewRenderPlan& plan,
    bool allocateStorage)
    : MemoryAtlasPageSink(atlasPlan, true),
      renderPlan_(plan),
      frameCount_(plan.frameCount()),
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

void MemoryFrameSink::afterPageReadback(std::uint64_t pageIndex) {
    const MultiviewAtlasPlan& plan = atlasPlan();
    const std::uint64_t framesOnPage = plan.frameCountOnPage(pageIndex);
    const std::uint64_t firstFrame = plan.firstFrameOnPage(pageIndex);
    const int tileSize = plan.tileSize();
    const int channels = renderPlan_.channels();
    const std::size_t rowBytes = static_cast<std::size_t>(tileSize) * channels;
    const unsigned char* page = pageData(pageIndex);

    for (std::uint64_t i = 0; i < framesOnPage; ++i) {
        const std::uint64_t frameIndex = firstFrame + i;
        const MultiviewAtlasTile tile = plan.tileForFrame(frameIndex);
        unsigned char* target = frameData(frameIndex);

        for (int row = 0; row < tileSize; ++row) {
            const std::size_t sourceOffset =
                (static_cast<std::size_t>(tile.y + row) *
                     static_cast<std::size_t>(plan.pageWidth()) +
                 static_cast<std::size_t>(tile.x)) *
                static_cast<std::size_t>(channels);
            std::memcpy(target + static_cast<std::size_t>(row) * rowBytes,
                        page + sourceOffset,
                        rowBytes);
        }
    }
}

std::uint64_t MemoryFrameSink::capturedBytes(std::uint64_t pageReadbacks) const {
    if (pageReadbacks == atlasPlan().pageCount()) {
        return totalBytes_;
    }
    return 0;
}

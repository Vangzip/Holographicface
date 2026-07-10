#include "memoryAtlasPageSink.h"

#include <limits>
#include <new>
#include <stdexcept>

MemoryAtlasPageSink::MemoryAtlasPageSink(const MultiviewAtlasPlan& atlasPlan, bool allocateStorage)
    : atlasPlan_(atlasPlan),
      pageBytes_(atlasPlan.pageBytes()),
      atlasBytes_(atlasPlan.atlasBytes())
{
    if (allocateStorage) {
        if (atlasBytes_ > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            throw std::length_error("atlas page buffer is too large for this platform");
        }
        buffer_.reset(new unsigned char[static_cast<std::size_t>(atlasBytes_)]);
    }
}

MemoryAtlasPageSink::~MemoryAtlasPageSink() = default;

std::uint64_t MemoryAtlasPageSink::pageCount() const
{
    return atlasPlan_.pageCount();
}

std::uint64_t MemoryAtlasPageSink::pageBytes() const
{
    return pageBytes_;
}

std::uint64_t MemoryAtlasPageSink::atlasBytes() const
{
    return atlasBytes_;
}

unsigned char* MemoryAtlasPageSink::pageData(std::uint64_t pageIndex)
{
    if (pageIndex >= atlasPlan_.pageCount()) {
        throw std::out_of_range("atlas page index out of range");
    }
    if (!buffer_) {
        throw std::logic_error("atlas page storage was not allocated");
    }
    return buffer_.get() + static_cast<std::size_t>(pageIndex * pageBytes_);
}

void MemoryAtlasPageSink::afterPageReadback(std::uint64_t)
{
}

std::uint64_t MemoryAtlasPageSink::capturedBytes(std::uint64_t pageReadbacks) const
{
    return pageReadbacks * pageBytes_;
}

const MultiviewAtlasPlan& MemoryAtlasPageSink::atlasPlan() const
{
    return atlasPlan_;
}

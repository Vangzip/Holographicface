#ifndef MEMORY_ATLAS_PAGE_SINK_H
#define MEMORY_ATLAS_PAGE_SINK_H

#include "multiviewAtlasPlan.h"

#include <cstdint>
#include <memory>

class MemoryAtlasPageSink {
public:
    MemoryAtlasPageSink(const MultiviewAtlasPlan& atlasPlan, bool allocateStorage);
    virtual ~MemoryAtlasPageSink();

    std::uint64_t pageCount() const;
    std::uint64_t pageBytes() const;
    std::uint64_t atlasBytes() const;

    virtual unsigned char* pageData(std::uint64_t pageIndex);
    virtual void afterPageReadback(std::uint64_t pageIndex);
    virtual std::uint64_t capturedBytes(std::uint64_t pageReadbacks) const;

protected:
    const MultiviewAtlasPlan& atlasPlan() const;

private:
    MultiviewAtlasPlan atlasPlan_;
    std::uint64_t pageBytes_;
    std::uint64_t atlasBytes_;
    std::unique_ptr<unsigned char[]> buffer_;
};

#endif

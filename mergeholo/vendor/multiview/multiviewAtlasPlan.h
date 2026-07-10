#ifndef MULTIVIEW_ATLAS_PLAN_H
#define MULTIVIEW_ATLAS_PLAN_H

#include "multiviewRenderPlan.h"

#include <cstdint>

struct MultiviewAtlasTile {
    std::uint64_t frameIndex;
    std::uint64_t pageIndex;
    int tileIndex;
    int tileRow;
    int tileColumn;
    int x;
    int y;
};

class MultiviewAtlasPlan {
public:
    MultiviewAtlasPlan(const MultiviewRenderPlan& renderPlan, int maxTextureSize);

    int tileSize() const;
    int maxTextureSize() const;
    int tilesPerAxis() const;
    int framesPerPage() const;
    int pageWidth() const;
    int pageHeight() const;
    std::uint64_t pageCount() const;
    std::uint64_t pageBytes() const;
    std::uint64_t atlasBytes() const;
    std::uint64_t totalBytes() const;
    std::uint64_t firstFrameOnPage(std::uint64_t pageIndex) const;
    std::uint64_t frameCountOnPage(std::uint64_t pageIndex) const;
    MultiviewAtlasTile tileForFrame(std::uint64_t frameIndex) const;

private:
    MultiviewRenderPlan renderPlan_;
    int maxTextureSize_;
    int tilesPerAxis_;
    int framesPerPage_;
};

#endif

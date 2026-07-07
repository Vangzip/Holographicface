#include "multiviewAtlasPlan.h"

#include <stdexcept>

namespace {
std::uint64_t ceilDiv(std::uint64_t value, std::uint64_t divisor) {
    return (value + divisor - 1) / divisor;
}
}

MultiviewAtlasPlan::MultiviewAtlasPlan(const MultiviewRenderPlan& renderPlan, int maxTextureSize)
    : renderPlan_(renderPlan),
      maxTextureSize_(maxTextureSize),
      tilesPerAxis_(0),
      framesPerPage_(0) {
    if (maxTextureSize <= 0) {
        throw std::invalid_argument("max texture size must be positive");
    }

    tilesPerAxis_ = maxTextureSize / renderPlan_.resolution();
    if (tilesPerAxis_ <= 0) {
        throw std::invalid_argument("max texture size is smaller than one tile");
    }
    framesPerPage_ = tilesPerAxis_ * tilesPerAxis_;
}

int MultiviewAtlasPlan::tileSize() const {
    return renderPlan_.resolution();
}

int MultiviewAtlasPlan::maxTextureSize() const {
    return maxTextureSize_;
}

int MultiviewAtlasPlan::tilesPerAxis() const {
    return tilesPerAxis_;
}

int MultiviewAtlasPlan::framesPerPage() const {
    return framesPerPage_;
}

int MultiviewAtlasPlan::pageWidth() const {
    return tilesPerAxis_ * tileSize();
}

int MultiviewAtlasPlan::pageHeight() const {
    return tilesPerAxis_ * tileSize();
}

std::uint64_t MultiviewAtlasPlan::pageCount() const {
    return ceilDiv(renderPlan_.frameCount(), static_cast<std::uint64_t>(framesPerPage_));
}

std::uint64_t MultiviewAtlasPlan::pageBytes() const {
    return static_cast<std::uint64_t>(pageWidth()) *
           static_cast<std::uint64_t>(pageHeight()) *
           static_cast<std::uint64_t>(renderPlan_.channels());
}

std::uint64_t MultiviewAtlasPlan::totalBytes() const {
    return renderPlan_.totalBytes();
}

std::uint64_t MultiviewAtlasPlan::firstFrameOnPage(std::uint64_t pageIndex) const {
    if (pageIndex >= pageCount()) {
        throw std::out_of_range("atlas page index out of range");
    }
    return pageIndex * static_cast<std::uint64_t>(framesPerPage_);
}

std::uint64_t MultiviewAtlasPlan::frameCountOnPage(std::uint64_t pageIndex) const {
    const std::uint64_t firstFrame = firstFrameOnPage(pageIndex);
    const std::uint64_t remaining = renderPlan_.frameCount() - firstFrame;
    return remaining < static_cast<std::uint64_t>(framesPerPage_)
               ? remaining
               : static_cast<std::uint64_t>(framesPerPage_);
}

MultiviewAtlasTile MultiviewAtlasPlan::tileForFrame(std::uint64_t frameIndex) const {
    if (frameIndex >= renderPlan_.frameCount()) {
        throw std::out_of_range("atlas frame index out of range");
    }

    const std::uint64_t pageIndex =
        frameIndex / static_cast<std::uint64_t>(framesPerPage_);
    const int tileIndex =
        static_cast<int>(frameIndex % static_cast<std::uint64_t>(framesPerPage_));
    const int tileRow = tileIndex / tilesPerAxis_;
    const int tileColumn = tileIndex % tilesPerAxis_;

    MultiviewAtlasTile tile;
    tile.frameIndex = frameIndex;
    tile.pageIndex = pageIndex;
    tile.tileIndex = tileIndex;
    tile.tileRow = tileRow;
    tile.tileColumn = tileColumn;
    tile.x = tileColumn * tileSize();
    tile.y = tileRow * tileSize();
    return tile;
}

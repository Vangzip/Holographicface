#include "ElementalAtlasDirectSink.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

namespace {

double secondsBetween(std::chrono::steady_clock::time_point start,
                      std::chrono::steady_clock::time_point end)
{
    return std::chrono::duration<double>(end - start).count();
}

} // namespace

ElementalAtlasDirectSink::ElementalAtlasDirectSink(
    const MultiviewAtlasPlan& atlasPlan,
    const MultiviewRenderPlan& renderPlan,
    const ElementalMemoryTransformConfig& config,
    ElementalMemoryResult* result)
    : MemoryAtlasPageSink(atlasPlan, false),
      atlasPlan_(atlasPlan),
      renderPlan_(renderPlan),
      config_(config),
      result_(result),
      pageBuffer_(static_cast<std::size_t>(atlasPlan.pageBytes()))
{
    if (result_ == nullptr) {
        throw std::invalid_argument("elemental result must not be null");
    }
    if (config_.viewRows != renderPlan_.samplesPerAxis()
        || config_.viewCols != renderPlan_.samplesPerAxis()
        || config_.sourceRows != renderPlan_.resolution()
        || config_.sourceCols != renderPlan_.resolution()) {
        throw std::invalid_argument("atlas direct dimensions do not match render plan");
    }

    std::size_t imageCount = 0;
    std::size_t imageBytes = 0;
    std::size_t totalBytes = 0;
    if (!computeElementalMemoryOutputSize(
            config_.viewRows, config_.viewCols, config_.targetRows, config_.targetCols,
            imageCount, imageBytes, totalBytes)) {
        throw std::length_error("elemental direct output size overflow");
    }

    result_->clear();
    result_->pixels.reset(new unsigned char[totalBytes]);
    result_->imageCount = imageCount;
    result_->imageBytes = imageBytes;
    result_->totalBytes = totalBytes;
    result_->rows = config_.viewRows;
    result_->cols = config_.viewCols;
    result_->targetRows = config_.targetRows;
    result_->targetCols = config_.targetCols;
    result_->sourceRows = config_.sourceRows;
    result_->sourceCols = config_.sourceCols;
    result_->flipSourceY = config_.flipSourceY;
    result_->flipViewRows = config_.flipViewRows;
    result_->sourceRowsBottomUp = config_.sourceRowsBottomUp;
    result_->mode = ElementalMemoryMode::Materialized;

    if (config_.sourceRows != config_.targetRows || config_.sourceCols != config_.targetCols) {
        std::memset(result_->pixels.get(), 0, totalBytes);
    }
}

unsigned char* ElementalAtlasDirectSink::pageData(std::uint64_t pageIndex)
{
    if (pageIndex >= atlasPlan_.pageCount()) {
        throw std::out_of_range("atlas page index out of range");
    }
    return pageBuffer_.data();
}

void ElementalAtlasDirectSink::afterPageReadback(std::uint64_t pageIndex)
{
    const auto start = std::chrono::steady_clock::now();
    scatterPage(pageIndex);
    scatterSeconds_ += secondsBetween(start, std::chrono::steady_clock::now());
}

double ElementalAtlasDirectSink::scatterSeconds() const
{
    return scatterSeconds_;
}

int ElementalAtlasDirectSink::sourceRowForTargetRow(int targetRow) const
{
    const int logicalSourceRow = config_.flipSourceY
        ? config_.sourceRows - 1 - targetRow
        : targetRow;
    return config_.sourceRowsBottomUp
        ? config_.sourceRows - 1 - logicalSourceRow
        : logicalSourceRow;
}

int ElementalAtlasDirectSink::outputViewForFrame(std::uint64_t frameIndex) const
{
    const int sourceViewRow = static_cast<int>(frameIndex / static_cast<std::uint64_t>(config_.viewCols));
    const int sourceViewCol = static_cast<int>(frameIndex % static_cast<std::uint64_t>(config_.viewCols));
    const int outputViewRow = config_.flipViewRows
        ? config_.viewRows - 1 - sourceViewRow
        : sourceViewRow;
    return outputViewRow * config_.viewCols + sourceViewCol;
}

void ElementalAtlasDirectSink::scatterPage(std::uint64_t pageIndex)
{
    if (result_ == nullptr || !result_->isMaterialized()) {
        throw std::logic_error("elemental result storage is not materialized");
    }

    const int channels = 3;
    const std::size_t outputImageBytes = result_->imageBytes;
    const std::uint64_t firstFrame = atlasPlan_.firstFrameOnPage(pageIndex);
    const std::uint64_t framesOnPage = atlasPlan_.frameCountOnPage(pageIndex);

    for (std::uint64_t localFrame = 0; localFrame < framesOnPage; ++localFrame) {
        const std::uint64_t frameIndex = firstFrame + localFrame;
        const MultiviewAtlasTile tile = atlasPlan_.tileForFrame(frameIndex);
        const int outputView = outputViewForFrame(frameIndex);
        const std::size_t outputViewOffset = static_cast<std::size_t>(outputView) * channels;

        for (int targetRow = 0; targetRow < config_.targetRows; ++targetRow) {
            const int sourceRow = sourceRowForTargetRow(targetRow);
            if (sourceRow < 0 || sourceRow >= config_.sourceRows) {
                continue;
            }
            const unsigned char* atlasRow = pageBuffer_.data()
                + (static_cast<std::size_t>(tile.y + sourceRow) * atlasPlan_.pageWidth()
                    + static_cast<std::size_t>(tile.x)) * channels;
            const std::size_t outputRowBase = static_cast<std::size_t>(targetRow)
                * static_cast<std::size_t>(config_.targetCols);
            const int copyCols = std::min(config_.targetCols, config_.sourceCols);

            for (int targetCol = 0; targetCol < copyCols; ++targetCol) {
                const std::size_t targetIndex = outputRowBase + static_cast<std::size_t>(targetCol);
                unsigned char* dst = result_->pixels.get()
                    + targetIndex * outputImageBytes
                    + outputViewOffset;
                const unsigned char* src = atlasRow + static_cast<std::size_t>(targetCol) * channels;
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
            }
        }
    }
}

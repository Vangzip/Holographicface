#pragma once

#include "ElementalMemoryResult.h"
#include "ElementalMemoryTransform.h"

#include "memoryAtlasPageSink.h"
#include "multiviewAtlasPlan.h"
#include "multiviewRenderPlan.h"

#include <chrono>
#include <cstdint>
#include <vector>

class ElementalAtlasDirectSink : public MemoryAtlasPageSink {
public:
    ElementalAtlasDirectSink(
        const MultiviewAtlasPlan& atlasPlan,
        const MultiviewRenderPlan& renderPlan,
        const ElementalMemoryTransformConfig& config,
        ElementalMemoryResult* result);

    unsigned char* pageData(std::uint64_t pageIndex) override;
    void afterPageReadback(std::uint64_t pageIndex) override;

    double scatterSeconds() const;

private:
    int sourceRowForTargetRow(int targetRow) const;
    int outputViewForFrame(std::uint64_t frameIndex) const;
    void scatterPage(std::uint64_t pageIndex);

    MultiviewAtlasPlan atlasPlan_;
    MultiviewRenderPlan renderPlan_;
    ElementalMemoryTransformConfig config_;
    ElementalMemoryResult* result_;
    std::vector<unsigned char> pageBuffer_;
    double scatterSeconds_ = 0.0;
};

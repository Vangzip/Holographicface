#pragma once

#include <cstddef>
#include <memory>

struct ElementalMemoryResult {
    std::unique_ptr<unsigned char[]> pixels;
    size_t imageCount = 0;
    size_t imageBytes = 0;
    size_t totalBytes = 0;
    int rows = 0;
    int cols = 0;
};

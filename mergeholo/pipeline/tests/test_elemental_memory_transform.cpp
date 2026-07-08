#include "../elemental/ElementalMemoryTransform.h"
#include "../elemental/ElementalMemoryResult.h"
#include "memoryFrameSink.h"
#include "multiviewRenderPlan.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "检查失败: " << message << "\n";
        std::exit(1);
    }
}

std::size_t frameOffset(int viewIndex, int sourceRows, int sourceCols, int row, int col) {
    return (static_cast<std::size_t>(viewIndex) * static_cast<std::size_t>(sourceRows) * static_cast<std::size_t>(sourceCols)
        + static_cast<std::size_t>(row) * static_cast<std::size_t>(sourceCols)
        + static_cast<std::size_t>(col)) * 3;
}

std::size_t outputOffset(int targetIndex, int viewRows, int viewCols, int viewRow, int viewCol) {
    return (static_cast<std::size_t>(targetIndex) * static_cast<std::size_t>(viewRows) * static_cast<std::size_t>(viewCols)
        + static_cast<std::size_t>(viewRow) * static_cast<std::size_t>(viewCols)
        + static_cast<std::size_t>(viewCol)) * 3;
}

std::vector<unsigned char> makeSource(int viewRows, int viewCols, int sourceRows, int sourceCols) {
    const int viewCount = viewRows * viewCols;
    std::vector<unsigned char> source(static_cast<std::size_t>(viewCount) * sourceRows * sourceCols * 3);
    for (int view = 0; view < viewCount; ++view) {
        for (int row = 0; row < sourceRows; ++row) {
            for (int col = 0; col < sourceCols; ++col) {
                const std::size_t offset = frameOffset(view, sourceRows, sourceCols, row, col);
                source[offset + 0] = static_cast<unsigned char>(view + 1);
                source[offset + 1] = static_cast<unsigned char>(row + 10);
                source[offset + 2] = static_cast<unsigned char>(col + 20);
            }
        }
    }
    return source;
}

void expectPixel(
    const std::vector<unsigned char>& output,
    int targetIndex,
    int viewRows,
    int viewCols,
    int viewRow,
    int viewCol,
    unsigned char r,
    unsigned char g,
    unsigned char b) {
    const std::size_t offset = outputOffset(targetIndex, viewRows, viewCols, viewRow, viewCol);
    expect(output[offset + 0] == r, "红色通道不匹配");
    expect(output[offset + 1] == g, "绿色通道不匹配");
    expect(output[offset + 2] == b, "蓝色通道不匹配");
}

ElementalMemoryTransformConfig baseConfig() {
    ElementalMemoryTransformConfig config;
    config.viewRows = 2;
    config.viewCols = 2;
    config.sourceRows = 2;
    config.sourceCols = 2;
    config.targetRows = 2;
    config.targetCols = 2;
    config.flipSourceY = false;
    config.flipViewRows = false;
    config.threadCount = 1;
    return config;
}

void testSizes() {
    std::size_t imageCount = 0;
    std::size_t imageBytes = 0;
    std::size_t totalBytes = 0;
    const bool ok = computeElementalMemoryOutputSize(2, 3, 4, 5, imageCount, imageBytes, totalBytes);
    expect(ok, "尺寸计算失败");
    expect(imageCount == 20, "输出图数量不匹配");
    expect(imageBytes == 18, "单张输出字节数不匹配");
    expect(totalBytes == 360, "总输出字节数不匹配");
}

void testDefaultSizeMathMatchesPipelineDefaults() {
    std::size_t imageCount = 0;
    std::size_t imageBytes = 0;
    std::size_t totalBytes = 0;
    const bool ok = computeElementalMemoryOutputSize(270, 270, 150, 150, imageCount, imageBytes, totalBytes);
    expect(ok, "默认尺寸计算失败");
    expect(imageCount == 22500, "默认输出图数量不匹配");
    expect(imageBytes == 218700, "默认单张输出字节数不匹配");
    expect(totalBytes == 4920750000ULL, "默认总输出字节数不匹配");
}

void testBasicMappingWithoutFlips() {
    ElementalMemoryTransformConfig config = baseConfig();
    const std::vector<unsigned char> source = makeSource(config.viewRows, config.viewCols, config.sourceRows, config.sourceCols);
    std::vector<unsigned char> output(4 * 4 * 3, 0);
    ElementalMemoryTransformStatus status = storeElementalFromMemory(
        source.data(), static_cast<std::size_t>(config.sourceRows) * config.sourceCols * 3,
        output.data(), config);
    expect(status == ElementalMemoryTransformStatus::Ok, "基础转换失败");

    expectPixel(output, 0, 2, 2, 0, 0, 1, 10, 20);
    expectPixel(output, 0, 2, 2, 0, 1, 2, 10, 20);
    expectPixel(output, 0, 2, 2, 1, 0, 3, 10, 20);
    expectPixel(output, 0, 2, 2, 1, 1, 4, 10, 20);
    expectPixel(output, 3, 2, 2, 0, 0, 1, 11, 21);
    expectPixel(output, 3, 2, 2, 1, 1, 4, 11, 21);
}

void testSourceYFlipAndViewRowFlip() {
    ElementalMemoryTransformConfig config = baseConfig();
    config.flipSourceY = true;
    config.flipViewRows = true;
    const std::vector<unsigned char> source = makeSource(config.viewRows, config.viewCols, config.sourceRows, config.sourceCols);
    std::vector<unsigned char> output(4 * 4 * 3, 0);
    ElementalMemoryTransformStatus status = storeElementalFromMemory(
        source.data(), static_cast<std::size_t>(config.sourceRows) * config.sourceCols * 3,
        output.data(), config);
    expect(status == ElementalMemoryTransformStatus::Ok, "翻转转换失败");

    expectPixel(output, 0, 2, 2, 0, 0, 3, 11, 20);
    expectPixel(output, 0, 2, 2, 0, 1, 4, 11, 20);
    expectPixel(output, 0, 2, 2, 1, 0, 1, 11, 20);
    expectPixel(output, 0, 2, 2, 1, 1, 2, 11, 20);
}

void testBottomUpSourceRowsMatchFilePathYFlip() {
    ElementalMemoryTransformConfig config = baseConfig();
    config.flipSourceY = true;
    config.sourceRowsBottomUp = true;
    const std::vector<unsigned char> source = makeSource(config.viewRows, config.viewCols, config.sourceRows, config.sourceCols);
    std::vector<unsigned char> output(4 * 4 * 3, 0);
    ElementalMemoryTransformStatus status = storeElementalFromMemory(
        source.data(), static_cast<std::size_t>(config.sourceRows) * config.sourceCols * 3,
        output.data(), config);
    expect(status == ElementalMemoryTransformStatus::Ok, "bottom-up 源行转换失败");

    expectPixel(output, 0, 2, 2, 0, 0, 1, 10, 20);
    expectPixel(output, 0, 2, 2, 1, 1, 4, 10, 20);
    expectPixel(output, 3, 2, 2, 0, 0, 1, 11, 21);
    expectPixel(output, 3, 2, 2, 1, 1, 4, 11, 21);
}

void testZeroPaddingWhenSourceSmallerThanTarget() {
    ElementalMemoryTransformConfig config;
    config.viewRows = 2;
    config.viewCols = 2;
    config.sourceRows = 1;
    config.sourceCols = 1;
    config.targetRows = 2;
    config.targetCols = 2;
    config.flipSourceY = false;
    config.flipViewRows = false;
    config.threadCount = 1;
    const std::vector<unsigned char> source = makeSource(config.viewRows, config.viewCols, config.sourceRows, config.sourceCols);
    std::vector<unsigned char> output(4 * 4 * 3, 255);
    ElementalMemoryTransformStatus status = storeElementalFromMemory(
        source.data(), static_cast<std::size_t>(config.sourceRows) * config.sourceCols * 3,
        output.data(), config);
    expect(status == ElementalMemoryTransformStatus::Ok, "补零转换失败");

    expectPixel(output, 0, 2, 2, 0, 0, 1, 10, 20);
    expectPixel(output, 1, 2, 2, 0, 0, 0, 0, 0);
    expectPixel(output, 2, 2, 2, 0, 0, 0, 0, 0);
    expectPixel(output, 3, 2, 2, 0, 0, 0, 0, 0);
}

void testThreadEquivalence() {
    ElementalMemoryTransformConfig config;
    config.viewRows = 5;
    config.viewCols = 4;
    config.sourceRows = 6;
    config.sourceCols = 7;
    config.targetRows = 6;
    config.targetCols = 7;
    config.flipSourceY = true;
    config.flipViewRows = true;
    config.threadCount = 1;
    const std::vector<unsigned char> source = makeSource(config.viewRows, config.viewCols, config.sourceRows, config.sourceCols);
    std::vector<unsigned char> single(static_cast<std::size_t>(config.targetRows) * config.targetCols * config.viewRows * config.viewCols * 3);
    std::vector<unsigned char> threaded(single.size());
    expect(storeElementalFromMemory(source.data(), static_cast<std::size_t>(config.sourceRows) * config.sourceCols * 3, single.data(), config)
        == ElementalMemoryTransformStatus::Ok, "单线程转换失败");
    config.threadCount = 4;
    expect(storeElementalFromMemory(source.data(), static_cast<std::size_t>(config.sourceRows) * config.sourceCols * 3, threaded.data(), config)
        == ElementalMemoryTransformStatus::Ok, "多线程转换失败");
    expect(single == threaded, "多线程输出和单线程输出不一致");
}

void testVirtualResultMatchesMaterializedTransform() {
    ElementalMemoryTransformConfig config = baseConfig();
    config.flipSourceY = true;
    config.flipViewRows = true;
    config.sourceRowsBottomUp = true;

    const std::vector<unsigned char> source = makeSource(
        config.viewRows,
        config.viewCols,
        config.sourceRows,
        config.sourceCols);
    const std::size_t frameBytes = static_cast<std::size_t>(config.sourceRows) * config.sourceCols * 3;
    std::vector<unsigned char> materialized(
        static_cast<std::size_t>(config.targetRows) * config.targetCols * config.viewRows * config.viewCols * 3);
    expect(storeElementalFromMemory(source.data(), frameBytes, materialized.data(), config)
        == ElementalMemoryTransformStatus::Ok, "materialized transform failed");

    std::shared_ptr<MemoryFrameSink> sink(new MemoryFrameSink(MultiviewRenderPlan(2, 1, 2), true));
    std::memcpy(sink->data(), source.data(), source.size());

    ElementalMemoryResult result;
    result.sourceSink = sink;
    result.imageCount = static_cast<std::size_t>(config.targetRows) * config.targetCols;
    result.imageBytes = static_cast<std::size_t>(config.viewRows) * config.viewCols * 3;
    result.totalBytes = materialized.size();
    result.rows = config.viewRows;
    result.cols = config.viewCols;
    result.targetRows = config.targetRows;
    result.targetCols = config.targetCols;
    result.sourceRows = config.sourceRows;
    result.sourceCols = config.sourceCols;
    result.sourceFrameBytes = frameBytes;
    result.flipSourceY = config.flipSourceY;
    result.flipViewRows = config.flipViewRows;
    result.sourceRowsBottomUp = config.sourceRowsBottomUp;
    result.mode = ElementalMemoryMode::VirtualFromMultiview;

    std::vector<unsigned char> oneImage(result.imageBytes);
    for (std::size_t index = 0; index < result.imageCount; ++index) {
        expect(result.copyImage(index, oneImage.data()), "virtual copyImage failed");
        const unsigned char* expected = materialized.data() + index * result.imageBytes;
        expect(std::memcmp(oneImage.data(), expected, result.imageBytes) == 0,
            "virtual image does not match materialized output");
    }

    expect(result.materialize(), "virtual materialize failed");
    expect(result.isMaterialized(), "result should be materialized");
    expect(std::memcmp(result.pixels.get(), materialized.data(), materialized.size()) == 0,
        "materialized virtual result does not match transform output");
}

void runBenchmarkSmoke() {
    ElementalMemoryTransformConfig config;
    config.viewRows = 32;
    config.viewCols = 32;
    config.sourceRows = 64;
    config.sourceCols = 64;
    config.targetRows = 64;
    config.targetCols = 64;
    config.flipSourceY = true;
    config.flipViewRows = true;
    config.threadCount = chooseElementalMemoryThreads(0, config.targetRows * config.targetCols);
    const std::vector<unsigned char> source = makeSource(config.viewRows, config.viewCols, config.sourceRows, config.sourceCols);
    std::vector<unsigned char> output(static_cast<std::size_t>(config.targetRows) * config.targetCols * config.viewRows * config.viewCols * 3);
    const auto start = std::chrono::steady_clock::now();
    const ElementalMemoryTransformStatus status = storeElementalFromMemory(
        source.data(), static_cast<std::size_t>(config.sourceRows) * config.sourceCols * 3,
        output.data(), config);
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    if (status != ElementalMemoryTransformStatus::Ok) {
        std::cerr << "benchmark smoke 转换失败\n";
        std::exit(1);
    }
    std::uint64_t checksum = 0;
    for (unsigned char value : output) {
        checksum += value;
    }
    std::cout << "benchmark_smoke_seconds=" << seconds
              << ", checksum=" << checksum << "\n";
}

void runMediumBenchmark() {
    ElementalMemoryTransformConfig config;
    config.viewRows = 90;
    config.viewCols = 90;
    config.sourceRows = 96;
    config.sourceCols = 96;
    config.targetRows = 96;
    config.targetCols = 96;
    config.flipSourceY = true;
    config.flipViewRows = true;
    config.threadCount = chooseElementalMemoryThreads(0, config.targetRows * config.targetCols);
    const std::vector<unsigned char> source = makeSource(config.viewRows, config.viewCols, config.sourceRows, config.sourceCols);
    std::vector<unsigned char> output(static_cast<std::size_t>(config.targetRows) * config.targetCols * config.viewRows * config.viewCols * 3);
    const auto start = std::chrono::steady_clock::now();
    const ElementalMemoryTransformStatus status = storeElementalFromMemory(
        source.data(), static_cast<std::size_t>(config.sourceRows) * config.sourceCols * 3,
        output.data(), config);
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    if (status != ElementalMemoryTransformStatus::Ok) {
        std::cerr << "medium benchmark 转换失败\n";
        std::exit(1);
    }
    std::uint64_t checksum = 0;
    for (unsigned char value : output) {
        checksum += value;
    }
    std::cout << "medium_benchmark_seconds=" << seconds
              << ", threads=" << config.threadCount
              << ", bytes=" << output.size()
              << ", checksum=" << checksum << "\n";
}

} // namespace

int main() {
    testSizes();
    testDefaultSizeMathMatchesPipelineDefaults();
    testBasicMappingWithoutFlips();
    testSourceYFlipAndViewRowFlip();
    testBottomUpSourceRowsMatchFilePathYFlip();
    testZeroPaddingWhenSourceSmallerThanTarget();
    testThreadEquivalence();
    testVirtualResultMatchesMaterializedTransform();
    runBenchmarkSmoke();
    runMediumBenchmark();
    std::cout << "elemental 内存转换测试通过\n";
    return 0;
}

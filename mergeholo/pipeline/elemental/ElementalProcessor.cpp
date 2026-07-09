#include "ElementalProcessor.h"

#include "ElementalMemoryTransform.h"
#include "PipelineTiming.h"

#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

class BoundedIntQueue {
public:
    explicit BoundedIntQueue(size_t capacity)
        : capacity_(std::max<size_t>(1, capacity)) {
    }

    void push(int value) {
        std::unique_lock<std::mutex> lock(mutex_);
        notFull_.wait(lock, [&] { return closed_ || queue_.size() < capacity_; });
        if (closed_) {
            return;
        }

        queue_.push_back(value);
        notEmpty_.notify_one();
    }

    bool pop(int& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        notEmpty_.wait(lock, [&] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) {
            return false;
        }

        value = queue_.front();
        queue_.pop_front();
        notFull_.notify_one();
        return true;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        notEmpty_.notify_all();
        notFull_.notify_all();
    }

private:
    const size_t capacity_;
    std::deque<int> queue_;
    std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    bool closed_ = false;
};

bool requireExists(const fs::path& path, const std::string& label) {
    if (!fs::exists(path)) {
        std::cerr << "[error] Missing " << label << ": " << path.string() << std::endl;
        return false;
    }
    return true;
}

std::string padNumber(int value, int digits) {
    std::ostringstream out;
    out << std::setw(digits) << std::setfill('0') << value;
    return out.str();
}

fs::path viewPath(const HoloConfig& config, int row, int col) {
    const std::string name = padNumber(row, config.viewNameDigits)
        + padNumber(col, config.viewNameDigits)
        + ".jpg";
    return config.multiviewOutDir / name;
}

int chooseElementalWriterThreads(int requested, int targetPixels) {
    return chooseElementalMemoryThreads(requested, targetPixels);
}

} // namespace

int processElemental(
    const HoloConfig& config,
    const CliOptions& options,
    const MultiviewMemoryResult* memoryResult,
    ElementalMemoryResult* elementalResult) {
    if (config.viewRows <= 0 || config.viewCols <= 0 || config.targetRows <= 0 || config.targetCols <= 0) {
        std::cerr << "[elemental] derived view grid and target grid must be positive." << std::endl;
        return 1;
    }

    if (options.dryRun) {
        std::cout << "[elemental] input views: " << config.viewRows << "x" << config.viewCols
                  << ", each view should be: " << config.multiviewResolution
                  << "x" << config.multiviewResolution << std::endl;
        std::cout << "[elemental] output images: " << config.targetRows << "x" << config.targetCols
                  << ", each output: " << config.viewCols << "x" << config.viewRows << std::endl;
        std::cout << "[elemental] output mode: memory, log file: " << config.logFile.string() << std::endl;
        std::cout << "[elemental] flip source Y: " << (config.elementalFlipSourceY ? "true" : "false")
                  << ", flip view rows: " << (config.elementalFlipViewRows ? "true" : "false") << std::endl;
        return 0;
    }

    const bool useMemoryViews = memoryResult != nullptr && memoryResult->sink && memoryResult->plan;
    int sourceRows = config.multiviewResolution;
    int sourceCols = config.multiviewResolution;
    cv::Mat sample;

    if (useMemoryViews) {
        sourceRows = memoryResult->plan->resolution();
        sourceCols = memoryResult->plan->resolution();
        if (memoryResult->plan->samplesPerAxis() != config.viewRows
            || memoryResult->plan->samplesPerAxis() != config.viewCols
            || memoryResult->sink->frameCount() != memoryResult->plan->frameCount()
            || memoryResult->sink->frameBytes() != memoryResult->plan->frameBytes()) {
            std::cerr << "[elemental] multiview memory buffer does not match pipeline dimensions." << std::endl;
            return 1;
        }
    }
    else {
        if (!requireExists(config.multiviewOutDir, "multiview_out_dir")) {
            return 1;
        }

        const fs::path firstView = viewPath(config, 1, 1);
        const fs::path lastView = viewPath(config, config.viewRows, config.viewCols);
        if (!requireExists(firstView, "first multiview image")
            || !requireExists(lastView, "last multiview image")) {
            return 1;
        }

        sample = cv::imread(firstView.string(), cv::IMREAD_COLOR);
        if (sample.empty()) {
            std::cerr << "[error] Cannot read sample image: " << firstView.string() << std::endl;
            return 1;
        }
        sourceRows = sample.rows;
        sourceCols = sample.cols;
    }

    if (sourceCols != config.targetCols || sourceRows != config.targetRows) {
        std::cerr << "[warn] View image size is " << sourceCols << "x" << sourceRows
                  << ", target grid is " << config.targetCols << "x" << config.targetRows
                  << ". The target grid should match each view image size." << std::endl;
    }

    size_t viewCount = 0;
    size_t targetPixelCount = 0;
    size_t viewImageBytes = 0;
    size_t totalViewBytes = 0;
    size_t outputImageBytes = 0;
    size_t totalElementalBytes = 0;
    if (!checkedMultiply(static_cast<size_t>(config.viewRows), static_cast<size_t>(config.viewCols), viewCount)
        || !checkedMultiply(static_cast<size_t>(config.targetRows), static_cast<size_t>(config.targetCols), targetPixelCount)
        || !checkedMultiply(targetPixelCount, static_cast<size_t>(3), viewImageBytes)
        || !checkedMultiply(viewCount, viewImageBytes, totalViewBytes)
        || !checkedMultiply(viewCount, static_cast<size_t>(3), outputImageBytes)
        || !checkedMultiply(targetPixelCount, outputImageBytes, totalElementalBytes)) {
        std::cerr << "[elemental] image dimensions are too large." << std::endl;
        return 1;
    }

    if (targetPixelCount > static_cast<size_t>(std::numeric_limits<int>::max())) {
        std::cerr << "[elemental] target image count is too large." << std::endl;
        return 1;
    }

    const int targetPixels = static_cast<int>(targetPixelCount);
    const int writerThreads = useMemoryViews
        ? std::min(static_cast<int>(targetPixelCount), chooseElementalWriterThreads(config.elementalWriterThreads, targetPixels))
        : 1;
    if (elementalResult == nullptr) {
        std::cerr << "[elemental] internal memory result is not available." << std::endl;
        return 1;
    }

    std::cout << "[elemental] input views: " << config.viewRows << "x" << config.viewCols
              << ", each view: " << sourceCols << "x" << sourceRows
              << (useMemoryViews ? " from memory" : " from files") << std::endl;
    std::cout << "[elemental] output images: " << config.targetRows << "x" << config.targetCols
              << ", each output: " << config.viewCols << "x" << config.viewRows << std::endl;
    std::cout << "[elemental] output mode: memory only, files written: 0" << std::endl;
    std::cout << "[elemental] view cache: " << viewCount << " images, "
              << formatBytes(totalViewBytes)
              << (useMemoryViews ? " from multiview memory buffer" : " loaded from files")
              << std::endl;
    std::cout << "[elemental] per-writer output buffer: " << formatBytes(outputImageBytes) << std::endl;
    std::cout << "[elemental] output memory: " << formatBytes(totalElementalBytes) << std::endl;
    std::cout << "[elemental] writer threads: " << writerThreads << std::endl;
    std::cout << "[elemental] flip source Y: " << (config.elementalFlipSourceY ? "true" : "false")
              << ", flip view rows: " << (config.elementalFlipViewRows ? "true" : "false") << std::endl;

    std::unique_ptr<unsigned char[]> viewPixels;
    if (!useMemoryViews) {
        try {
            viewPixels.reset(new unsigned char[totalViewBytes]);
        }
        catch (const std::bad_alloc&) {
            std::cerr << "[elemental] cannot allocate view cache: "
                      << formatBytes(totalViewBytes) << std::endl;
            return 1;
        }
    }

    const auto loadStart = std::chrono::steady_clock::now();
    const size_t targetRowBytes = static_cast<size_t>(config.targetCols) * 3;
    int mismatchedViewImages = 0;
    const int loadProgressEvery = std::max(1, config.viewRows / 10);

    if (!useMemoryViews) {
        for (int viewRow = 1; viewRow <= config.viewRows; ++viewRow) {
            for (int viewCol = 1; viewCol <= config.viewCols; ++viewCol) {
                const size_t viewIndex = static_cast<size_t>(viewRow - 1) * static_cast<size_t>(config.viewCols)
                    + static_cast<size_t>(viewCol - 1);
                unsigned char* viewBase = viewPixels.get() + viewIndex * viewImageBytes;
                std::fill(viewBase, viewBase + viewImageBytes, 0);
                const int copyCols = std::min(config.targetCols, sourceCols);
                const size_t copyBytes = static_cast<size_t>(copyCols) * 3;
                cv::Mat input;
                if (viewRow == 1 && viewCol == 1) {
                    input = sample;
                }
                else {
                    const fs::path inputPath = viewPath(config, viewRow, viewCol);
                    input = cv::imread(inputPath.string(), cv::IMREAD_COLOR);
                    if (input.empty()) {
                        std::cerr << "[error] Cannot read multiview image: " << inputPath.string() << std::endl;
                        return 1;
                    }
                }

                const bool mismatchedSize = input.rows != config.targetRows || input.cols != config.targetCols;
                if (mismatchedSize) {
                    ++mismatchedViewImages;
                }
                for (int targetRow = 0; targetRow < config.targetRows; ++targetRow) {
                    const int sourceRow = config.elementalFlipSourceY
                        ? input.rows - 1 - targetRow
                        : targetRow;
                    if (sourceRow < 0 || sourceRow >= input.rows) {
                        continue;
                    }
                    std::memcpy(
                        viewBase + static_cast<size_t>(targetRow) * targetRowBytes,
                        input.ptr<unsigned char>(sourceRow),
                        copyBytes);
                }
            }

            if (viewRow % loadProgressEvery == 0 || viewRow == config.viewRows) {
                std::cout << "[elemental] loaded view rows " << viewRow << "/"
                          << config.viewRows << std::endl;
            }
        }
    }

    if (mismatchedViewImages > 0) {
        std::cout << "[elemental] warning: " << mismatchedViewImages
                  << " view images did not match target grid "
                  << config.targetCols << "x" << config.targetRows << std::endl;
    }
    if (useMemoryViews) {
        std::cout << "[elemental] using multiview memory buffer directly; no file load or duplicate view cache." << std::endl;
    }
    else {
        std::cout << "[elemental] loaded view cache in "
                  << formatSeconds(elapsedSeconds(loadStart)) << "s" << std::endl;
    }

    elementalResult->clear();
    elementalResult->imageCount = targetPixelCount;
    elementalResult->imageBytes = outputImageBytes;
    elementalResult->totalBytes = totalElementalBytes;
    elementalResult->rows = config.viewRows;
    elementalResult->cols = config.viewCols;
    elementalResult->targetRows = config.targetRows;
    elementalResult->targetCols = config.targetCols;
    elementalResult->sourceRows = sourceRows;
    elementalResult->sourceCols = sourceCols;
    elementalResult->flipSourceY = config.elementalFlipSourceY;
    elementalResult->flipViewRows = config.elementalFlipViewRows;
    elementalResult->sourceRowsBottomUp = true;

    try {
        elementalResult->pixels.reset(new unsigned char[totalElementalBytes]);
        elementalResult->mode = ElementalMemoryMode::Materialized;
    }
    catch (const std::bad_alloc&) {
        std::cerr << "[elemental] cannot allocate output memory: "
                  << formatBytes(totalElementalBytes) << std::endl;
        elementalResult->clear();
        return 1;
    }

    const auto storeStart = std::chrono::steady_clock::now();
    if (useMemoryViews) {
        ElementalMemoryTransformConfig transformConfig;
        transformConfig.viewRows = config.viewRows;
        transformConfig.viewCols = config.viewCols;
        transformConfig.sourceRows = sourceRows;
        transformConfig.sourceCols = sourceCols;
        transformConfig.targetRows = config.targetRows;
        transformConfig.targetCols = config.targetCols;
        transformConfig.flipSourceY = config.elementalFlipSourceY;
        transformConfig.flipViewRows = config.elementalFlipViewRows;
        transformConfig.sourceRowsBottomUp = true;
        transformConfig.threadCount = writerThreads;

        const ElementalMemoryTransformStatus status = storeElementalFromMemoryBlocked(
            memoryResult->sink->data(),
            static_cast<size_t>(memoryResult->sink->frameBytes()),
            elementalResult->pixels.get(),
            transformConfig);
        if (status != ElementalMemoryTransformStatus::Ok) {
            std::cerr << "[elemental] memory transform failed." << std::endl;
            elementalResult->clear();
            return 1;
        }

        std::cout << "[elemental] stored output images in memory in "
                  << formatSeconds(elapsedSeconds(storeStart)) << "s" << std::endl;
        return 0;
    }

    std::atomic<int> completedImages(0);
    std::atomic<bool> failed(false);
    std::mutex logMutex;
    std::mutex errorMutex;
    std::string firstError;
    const int progressEvery = std::max(1, targetPixels / 10);

    auto recordError = [&](const std::string& message) {
        if (!failed.exchange(true)) {
            std::lock_guard<std::mutex> lock(errorMutex);
            firstError = message;
        }
    };

        auto writeOne = [&](int targetIndex) {
            if (!failed.load()) {
                try {
                    unsigned char* outputBase = elementalResult->pixels.get()
                        + static_cast<size_t>(targetIndex) * outputImageBytes;
                    std::memset(outputBase, 0, outputImageBytes);
                    const int targetRow = targetIndex / config.targetCols;
                    const int targetCol = targetIndex % config.targetCols;
                    const size_t targetOffset = (static_cast<size_t>(targetRow) * static_cast<size_t>(config.targetCols)
                        + static_cast<size_t>(targetCol)) * 3;

                    for (int outputViewRow = 0; outputViewRow < config.viewRows; ++outputViewRow) {
                        const int sourceViewRow = config.elementalFlipViewRows
                            ? config.viewRows - 1 - outputViewRow
                            : outputViewRow;
                        unsigned char* dst = outputBase
                            + static_cast<size_t>(outputViewRow) * static_cast<size_t>(config.viewCols) * 3;
                        const size_t sourceViewRowOffset = static_cast<size_t>(sourceViewRow)
                            * static_cast<size_t>(config.viewCols);
                        for (int outputViewCol = 0; outputViewCol < config.viewCols; ++outputViewCol) {
                            const size_t viewIndex = sourceViewRowOffset + static_cast<size_t>(outputViewCol);
                            const unsigned char* src = viewPixels.get() + viewIndex * viewImageBytes + targetOffset;
                            dst[0] = src[0];
                            dst[1] = src[1];
                            dst[2] = src[2];
                            dst += 3;
                        }
                    }
                }
                catch (const std::exception& ex) {
                    recordError(std::string("[error] Elemental memory store failed: ") + ex.what());
                }
        }

        const int done = ++completedImages;
        if (done % progressEvery == 0 || done == targetPixels) {
            if (writerThreads == 1) {
                std::cout << "[elemental] stored " << done << "/" << targetPixels
                          << " images in memory" << std::endl;
            }
            else {
                std::lock_guard<std::mutex> lock(logMutex);
                std::cout << "[elemental] stored " << done << "/" << targetPixels
                          << " images in memory" << std::endl;
            }
        }
    };

    if (writerThreads == 1) {
        for (int targetIndex = 0; targetIndex < targetPixels && !failed.load(); ++targetIndex) {
            writeOne(targetIndex);
        }
    }
    else {
        BoundedIntQueue jobs(static_cast<size_t>(std::max(16, writerThreads * 4)));
        auto writer = [&] {
            int targetIndex = 0;
            while (jobs.pop(targetIndex)) {
                writeOne(targetIndex);
            }
        };

        std::vector<std::thread> workers;
        workers.reserve(static_cast<size_t>(writerThreads));
        try {
            for (int i = 0; i < writerThreads; ++i) {
                workers.emplace_back(writer);
            }
        }
        catch (const std::exception& ex) {
            jobs.close();
            for (std::thread& worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
            std::cerr << "[elemental] cannot start writer thread: " << ex.what() << std::endl;
            return 1;
        }

        for (int targetIndex = 0; targetIndex < targetPixels; ++targetIndex) {
            jobs.push(targetIndex);
        }
        jobs.close();

        for (std::thread& worker : workers) {
            worker.join();
        }
    }

    if (failed.load()) {
        std::lock_guard<std::mutex> lock(errorMutex);
        std::cerr << firstError << std::endl;
        return 1;
    }

    std::cout << "[elemental] stored output images in memory in "
              << formatSeconds(elapsedSeconds(storeStart)) << "s" << std::endl;
    return 0;
}

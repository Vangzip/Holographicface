#include "ResultPersistence.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <pcl/io/ply_io.h>

namespace fs = std::filesystem;

namespace {

std::string padNumber(int value, int digits)
{
    std::ostringstream output;
    output << std::setw(digits) << std::setfill('0') << value;
    return output.str();
}

int decimalDigits(int value)
{
    int digits = 1;
    while (value >= 10) {
        value /= 10;
        ++digits;
    }
    return digits;
}

void recordWarningNoThrow(
    const char* resultType,
    const fs::path& outputDirectory,
    ResultSaveReport& report,
    const char* message) noexcept
{
    try {
        report.addWarning(resultType, outputDirectory, message);
    }
    catch (...) {
    }
    try {
        std::cerr << "[save][warning] " << resultType << ": "
                  << outputDirectory.string() << ": " << message << std::endl;
    }
    catch (...) {
    }
}

template <typename Writer>
void runBestEffort(
    const char* resultType,
    const fs::path& baseDirectory,
    const std::string& timestamp,
    ResultSaveReport& report,
    Writer&& writer) noexcept
{
    fs::path outputDirectory = baseDirectory;
    try {
        outputDirectory = timestampedResultDirectory(baseDirectory, timestamp);
        if (timestamp.empty()) {
            throw std::invalid_argument("result_timestamp is empty");
        }
        fs::create_directories(outputDirectory);
        writer(outputDirectory);
    }
    catch (const std::exception& ex) {
        recordWarningNoThrow(resultType, outputDirectory, report, ex.what());
    }
    catch (...) {
        recordWarningNoThrow(
            resultType, outputDirectory, report, "unknown persistence error");
    }
}

void writeJpeg(
    const fs::path& outputPath,
    const cv::Mat& bgrImage,
    int quality)
{
    const std::vector<int> parameters = {
        cv::IMWRITE_JPEG_QUALITY,
        quality
    };
    if (!cv::imwrite(outputPath.string(), bgrImage, parameters)) {
        throw std::runtime_error("cannot write JPEG: " + outputPath.string());
    }
}

} // namespace

fs::path timestampedResultDirectory(
    const fs::path& baseDirectory,
    const std::string& timestamp)
{
    if (timestamp.empty()) {
        return baseDirectory;
    }
    return baseDirectory.parent_path()
        / (baseDirectory.filename().string() + "_" + timestamp);
}

void persistMeshResult(
    const HoloConfig& config,
    const MeshMemoryResult& result,
    ResultSaveReport& report) noexcept
{
    runBestEffort("mesh", config.meshOutDir, config.resultTimestamp, report,
        [&](const fs::path& outputDirectory) {
            if (!result.hasMesh()) {
                if (result.meshPath.empty() || !fs::is_regular_file(result.meshPath)) {
                    throw std::invalid_argument("mesh result is empty");
                }
                fs::copy_file(
                    result.meshPath,
                    outputDirectory / "0_mesh.ply",
                    fs::copy_options::overwrite_existing);
                return;
            }
            const fs::path outputPath = outputDirectory / "0_mesh.ply";
            if (pcl::io::savePLYFile(outputPath.string(), *result.mesh) != 0) {
                throw std::runtime_error("cannot write PLY: " + outputPath.string());
            }
        });
}

void persistMultiviewResult(
    const HoloConfig& config,
    const MultiviewMemoryResult& result,
    ResultSaveReport& report) noexcept
{
    runBestEffort("multiview", config.multiviewOutDir, config.resultTimestamp, report,
        [&](const fs::path& outputDirectory) {
            if (!result.plan || !result.sink || !result.sink->data()) {
                throw std::invalid_argument("multiview memory result is empty");
            }
            if (result.sink->frameCount() != result.plan->frameCount()
                || result.sink->frameBytes() != result.plan->frameBytes()) {
                throw std::invalid_argument("multiview memory result dimensions do not match");
            }

            const int resolution = result.plan->resolution();
            for (std::uint64_t index = 0; index < result.plan->frameCount(); ++index) {
                const MultiviewFrame frame = result.plan->frameAt(index);
                cv::Mat rgb(resolution, resolution, CV_8UC3, result.sink->frameData(index));
                cv::Mat orientedRgb;
                if (config.multiviewCamera.captureFlipVertical) {
                    cv::flip(rgb, orientedRgb, 0);
                }
                else {
                    orientedRgb = rgb;
                }
                cv::Mat bgr;
                cv::cvtColor(orientedRgb, bgr, cv::COLOR_RGB2BGR);

                const std::string name = padNumber(frame.row + 1, config.viewNameDigits)
                    + padNumber(frame.column + 1, config.viewNameDigits)
                    + ".jpg";
                writeJpeg(outputDirectory / name, bgr, config.jpgQuality);
            }
        });
}

void persistElementalResult(
    const HoloConfig& config,
    const ElementalMemoryResult& result,
    ResultSaveReport& report) noexcept
{
    runBestEffort("elemental", config.elementalOutDir, config.resultTimestamp, report,
        [&](const fs::path& outputDirectory) {
            if (!result.isMaterialized() || result.rows <= 0 || result.cols <= 0
                || result.targetRows <= 0 || result.targetCols <= 0) {
                throw std::invalid_argument("elemental memory result is empty");
            }
            const size_t expectedImageCount = static_cast<size_t>(result.targetRows)
                * static_cast<size_t>(result.targetCols);
            const size_t expectedImageBytes = static_cast<size_t>(result.rows)
                * static_cast<size_t>(result.cols) * 3;
            if (result.imageCount != expectedImageCount
                || result.imageBytes != expectedImageBytes
                || result.totalBytes < expectedImageCount * expectedImageBytes) {
                throw std::invalid_argument("elemental memory result dimensions do not match");
            }

            const int rowDigits = decimalDigits(result.targetRows);
            const int columnDigits = decimalDigits(result.targetCols);
            for (size_t index = 0; index < result.imageCount; ++index) {
                const int row = static_cast<int>(index / static_cast<size_t>(result.targetCols));
                const int column = static_cast<int>(index % static_cast<size_t>(result.targetCols));
                unsigned char* pixels = result.pixels.get() + index * result.imageBytes;
                cv::Mat rgb(result.rows, result.cols, CV_8UC3, pixels);
                cv::Mat bgr;
                cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);

                const std::string name = padNumber(row + 1, rowDigits)
                    + padNumber(column + 1, columnDigits)
                    + ".jpg";
                writeJpeg(outputDirectory / name, bgr, config.jpgQuality);
            }
        });
}

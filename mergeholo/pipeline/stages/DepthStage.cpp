#include "DepthStage.h"

#include "depth_io.h"
#include "FileLibrary.h"

#include <filesystem>
#include <iostream>
#include <list>
#include <string>

namespace fs = std::filesystem;

namespace {

bool requireExists(const fs::path& path, const std::string& label)
{
    if (!fs::exists(path)) {
        std::cerr << "[error] Missing " << label << ": " << path.string() << std::endl;
        return false;
    }
    return true;
}

std::string baseNameFromTiff(const std::string& depthFile)
{
    std::string filename = FileLibrary::getInstance()->getFileNameFromPath(depthFile);
    const size_t extPos = filename.rfind(".tiff");
    if (extPos != std::string::npos) {
        filename = filename.substr(0, extPos);
    }
    return filename;
}

} // namespace

int runDepthStage(const HoloConfig& config, const CliOptions& options)
{
    return runDepthStage(config, options, nullptr);
}

int runDepthStage(const HoloConfig& config, const CliOptions& options, DepthMemoryResult* memoryResult)
{
    if (!requireExists(config.depthInputDir, "depth_input_dir")
        || !requireExists(config.depthConfig, "depth_config")) {
        return 1;
    }

    if (options.dryRun) {
        std::cout << "[depth] scan " << config.depthInputDir.string()
                  << " and write *_rgb.ply with " << config.depthConfig.string() << std::endl;
        return 0;
    }

    std::list<std::string> files;
    FileLibrary::getInstance()->getAllSubFiles(config.depthInputDir.string(), files, false, true, false, ".tiff");
    if (files.empty()) {
        std::cerr << "[depth] no .tiff files found in " << config.depthInputDir.string() << std::endl;
        return 1;
    }

    if (memoryResult) {
        memoryResult->clear();
    }

    depthImage depth(0);
    int failed = 0;
    if (memoryResult && files.size() > 1) {
        std::cerr << "[depth] memory pipeline expects one .tiff file, found "
                  << files.size() << "." << std::endl;
        return 1;
    }

    for (const std::string& depthFile : files) {
        const std::string filename = baseNameFromTiff(depthFile);

        const std::string rgbFile = FileLibrary::getInstance()->combineFilePath(
            FileLibrary::getInstance()->getFileParentPath(depthFile), filename + ".jpg");

        std::cout << "[depth] " << depthFile << " + " << rgbFile << std::endl;
        if (!FileLibrary::getInstance()->isFileExists(rgbFile)) {
            std::cerr << "[depth] missing RGB image: " << rgbFile << std::endl;
            ++failed;
            continue;
        }

        if (memoryResult) {
            memoryResult->baseName = filename;
            memoryResult->rgbPath = rgbFile;
            memoryResult->pointCloudPath = config.depthInputDir / (filename + "_rgb.ply");
            memoryResult->cloud = depth.depthToPointCloudColor(depthFile, rgbFile, config.depthConfig.string());
            if (!memoryResult->hasCloud()) {
                ++failed;
            }
            continue;
        }

        if (!depth.depthToPlyColor(depthFile, rgbFile, config.depthConfig.string(), config.depthInputDir.string())) {
            ++failed;
        }
    }

    return failed == 0 ? 0 : 1;
}

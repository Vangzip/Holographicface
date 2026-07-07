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

} // namespace

int runDepthStage(const HoloConfig& config, const CliOptions& options)
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

    depthImage depth(0);
    int failed = 0;
    for (const std::string& depthFile : files) {
        std::string filename = FileLibrary::getInstance()->getFileNameFromPath(depthFile);
        const size_t extPos = filename.rfind(".tiff");
        if (extPos != std::string::npos) {
            filename = filename.substr(0, extPos);
        }

        const std::string rgbFile = FileLibrary::getInstance()->combineFilePath(
            FileLibrary::getInstance()->getFileParentPath(depthFile), filename + ".jpg");

        std::cout << "[depth] " << depthFile << " + " << rgbFile << std::endl;
        if (!FileLibrary::getInstance()->isFileExists(rgbFile)) {
            std::cerr << "[depth] missing RGB image: " << rgbFile << std::endl;
            ++failed;
            continue;
        }

        if (!depth.depthToPlyColor(depthFile, rgbFile, config.depthConfig.string(), config.depthInputDir.string())) {
            ++failed;
        }
    }

    return failed == 0 ? 0 : 1;
}

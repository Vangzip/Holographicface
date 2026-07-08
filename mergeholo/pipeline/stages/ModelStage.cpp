#include "ModelStage.h"

#include "ConverPointCloud.h"
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

int runModelStage(const HoloConfig& config, const CliOptions& options)
{
    return runModelStage(config, options, nullptr);
}

int runModelStage(const HoloConfig& config, const CliOptions& options, const MeshMemoryResult* meshMemory)
{
    if (!requireExists(config.depthInputDir, "depth_input_dir")
        || !requireExists(config.meshConfig, "mesh_config")) {
        return 1;
    }

    if (meshMemory && meshMemory->hasMesh()) {
        if (options.dryRun) {
            std::cout << "[model] memory " << meshMemory->meshPath.string()
                      << " and write textured OBJ models" << std::endl;
            return 0;
        }

        ConverPointCloud converter;
        std::cout << "[model] memory " << meshMemory->meshPath.string() << std::endl;
        return converter.modelAPIFromMesh(
            *meshMemory->mesh,
            meshMemory->meshPath.string(),
            meshMemory->rgbPath.string(),
            config.meshConfig.string()) ? 0 : 1;
    }

    if (options.dryRun) {
        std::cout << "[model] scan " << config.depthInputDir.string()
                  << " and write textured OBJ models" << std::endl;
        return 0;
    }

    std::list<std::string> files;
    FileLibrary::getInstance()->getAllSubFiles(config.depthInputDir.string(), files, false, true, false, "_mesh.ply");
    if (files.empty()) {
        std::cerr << "[model] no *_mesh.ply files found in " << config.depthInputDir.string() << std::endl;
        return 1;
    }

    ConverPointCloud converter;
    int failed = 0;
    for (const std::string& meshFile : files) {
        std::cout << "[model] " << meshFile << std::endl;
        if (!converter.modelAPI(meshFile, config.meshConfig.string())) {
            ++failed;
        }
    }

    return failed == 0 ? 0 : 1;
}

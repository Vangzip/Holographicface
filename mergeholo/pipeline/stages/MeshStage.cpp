#include "MeshStage.h"

#include "ConverPointCloud.h"
#include "FileLibrary.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <list>
#include <string>

namespace fs = std::filesystem;

namespace {

std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool requireExists(const fs::path& path, const std::string& label)
{
    if (!fs::exists(path)) {
        std::cerr << "[error] Missing " << label << ": " << path.string() << std::endl;
        return false;
    }
    return true;
}

fs::path meshOutputPath(const HoloConfig& config, const fs::path& inputPath)
{
    std::string baseName = inputPath.filename().string();
    const std::string suffix = "_rgb.ply";
    const std::string lowerName = lower(baseName);
    if (lowerName.size() >= suffix.size()
        && lowerName.compare(lowerName.size() - suffix.size(), suffix.size(), suffix) == 0) {
        baseName = baseName.substr(0, baseName.size() - suffix.size());
    }
    else {
        baseName = inputPath.stem().string();
    }

    return config.depthInputDir / (baseName + "_mesh.ply");
}

int runSingleMeshInput(
    const HoloConfig& config,
    const fs::path& inputPath,
    bool dryRun,
    const std::string& label,
    MeshMemoryResult* meshMemory)
{
    if (!requireExists(inputPath, label + " input")
        || !requireExists(config.meshConfig, "mesh_config")) {
        return 1;
    }

    const fs::path outputPath = meshOutputPath(config, inputPath);
    if (dryRun) {
        std::cout << "[" << label << "] " << inputPath.string()
                  << " -> " << outputPath.string() << std::endl;
        return 0;
    }

    ConverPointCloud converter;
    std::cout << "[" << label << "] " << inputPath.string() << std::endl;
    const bool ok = converter.meshAPI(
        inputPath.string(),
        config.meshConfig.string(),
        config.depthInputDir.string());
    if (!ok) {
        return 1;
    }
    if (!fs::exists(outputPath)) {
        std::cerr << "[" << label << "] expected output was not created: "
                  << outputPath.string() << std::endl;
        return 1;
    }

    if (meshMemory) {
        meshMemory->baseName = outputPath.stem().string();
        meshMemory->meshPath = outputPath;
    }
    return 0;
}

} // namespace

int runMeshOneStage(const HoloConfig& config, const CliOptions& options)
{
    if (options.inputPath.empty()) {
        std::cerr << "[mesh-one] --input is required." << std::endl;
        return 1;
    }

    return runSingleMeshInput(config, options.inputPath, options.dryRun, "mesh-one", nullptr);
}

int runMeshStage(const HoloConfig& config, const CliOptions& options)
{
    return runMeshStage(config, options, nullptr, nullptr);
}

int runMeshStage(
    const HoloConfig& config,
    const CliOptions& options,
    const DepthMemoryResult* depthMemory,
    MeshMemoryResult* meshMemory)
{
    if (!requireExists(config.depthInputDir, "depth_input_dir")
        || !requireExists(config.meshConfig, "mesh_config")) {
        return 1;
    }

    if (meshMemory) {
        meshMemory->clear();
    }

    if (depthMemory && depthMemory->hasCloud()) {
        const fs::path outputPath = meshOutputPath(config, depthMemory->pointCloudPath);
        if (options.dryRun) {
            std::cout << "[mesh] memory " << depthMemory->pointCloudPath.string()
                      << " -> " << outputPath.string() << std::endl;
            return 0;
        }

        ConverPointCloud converter;
        std::cout << "[mesh] memory " << depthMemory->pointCloudPath.string() << std::endl;
        pcl::PolygonMesh mesh;
        const bool keepMeshInMemory = meshMemory != nullptr;
        const bool ok = converter.meshAPIFromCloud(
            depthMemory->cloud,
            depthMemory->pointCloudPath.string(),
            config.meshConfig.string(),
            config.depthInputDir.string(),
            keepMeshInMemory ? &mesh : nullptr,
            !keepMeshInMemory);
        if (!ok) {
            return 1;
        }

        if (keepMeshInMemory) {
            meshMemory->baseName = depthMemory->baseName;
            meshMemory->meshPath = outputPath;
            meshMemory->rgbPath = depthMemory->rgbPath;
            meshMemory->mesh = pcl::PolygonMesh::Ptr(new pcl::PolygonMesh(mesh));
            if (!meshMemory->hasMesh()) {
                std::cerr << "[mesh] memory mesh was not created." << std::endl;
                return 1;
            }
            return 0;
        }

        if (!fs::exists(outputPath)) {
            std::cerr << "[mesh] expected output was not created: "
                      << outputPath.string() << std::endl;
            return 1;
        }
        return 0;
    }

    fs::path inputPath = options.inputPath;
    if (inputPath.empty()) {
        std::list<std::string> files;
        FileLibrary::getInstance()->getAllSubFiles(config.depthInputDir.string(), files, false, true, false, "_rgb.ply");
        if (files.empty()) {
            std::cerr << "[mesh] no *_rgb.ply files found in "
                      << config.depthInputDir.string() << std::endl;
            return 1;
        }
        if (files.size() > 1) {
            std::cerr << "[mesh] found " << files.size()
                      << " *_rgb.ply files; pass --input to choose one." << std::endl;
            return 1;
        }
        inputPath = files.front();
    }

    return runSingleMeshInput(config, inputPath, options.dryRun, "mesh", meshMemory);
}

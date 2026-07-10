#include "HoloPipeline.h"

#include "DepthStage.h"
#include "DepthMeshModelMemory.h"
#include "ElementalStage.h"
#include "MeshStage.h"
#include "ModelStage.h"
#include "MultiviewStage.h"
#include "PipelineConfig.h"
#include "PipelineContext.h"
#include "PipelineLogger.h"
#include "PipelineTiming.h"

#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

bool shouldRunStage(const CliOptions& options, const std::string& stageName)
{
    return options.stage == "all" || options.stage == stageName;
}

int runPipeline(HoloConfig& config, const CliOptions& options, ElementalMemoryResult* elementalOutput)
{
    const auto pipelineStart = std::chrono::steady_clock::now();
    std::vector<StageTiming> timings;
    DepthMemoryResult depthMemory;
    MeshMemoryResult meshMemory;
    MultiviewMemoryResult multiviewMemory;
    ElementalMemoryResult elementalMemory;
    bool skippedModelForMemoryMultiview = false;

    const bool runDepth = shouldRunStage(options, "depth") && config.runDepthPointCloud;
    const bool runMesh = shouldRunStage(options, "mesh") && config.runMesh;
    const bool runModel = shouldRunStage(options, "model") && config.runTexturedModel;
    const bool willRunMultiview = shouldRunStage(options, "multiview") && config.runMultiview;
    const bool useDepthMemory = runDepth && runMesh && options.inputPath.empty();
    const bool useMeshForModel = useDepthMemory && runModel;
    const bool useMeshMemory = useDepthMemory && (runModel || willRunMultiview);

    auto finish = [&](int code) {
        printTimingSummary(timings);
        const double wallSeconds = elapsedSeconds(pipelineStart);
        writePipelineLog(
            config,
            timings,
            multiviewMemory,
            elementalMemory,
            useDepthMemory,
            useMeshForModel && !skippedModelForMemoryMultiview,
            code,
            wallSeconds);
        std::cout << "[log] pipeline log: " << config.logFile.string() << std::endl;
        if (code == 0 && elementalOutput != nullptr) {
            *elementalOutput = std::move(elementalMemory);
        }
        return code;
    };

    if (options.stage == "mesh-one") {
        const int code = runTimedStage("mesh-one", [&] { return runMeshOneStage(config, options); }, timings);
        return finish(code);
    }

    if (runDepth) {
        const int code = runTimedStage("depth", [&] {
            return runDepthStage(config, options, useDepthMemory ? &depthMemory : nullptr);
        }, timings);
        if (code != 0) {
            depthMemory.clear();
            return finish(code);
        }
    }

    if (runMesh) {
        const int code = runTimedStage("mesh", [&] {
            return runMeshStage(
                config,
                options,
                depthMemory.hasCloud() ? &depthMemory : nullptr,
                useMeshMemory ? &meshMemory : nullptr);
        }, timings);
        depthMemory.clear();
        if (code != 0) {
            meshMemory.clear();
            return finish(code);
        }
    }

    if (runModel) {
        const bool skipModelForMemoryMultiview =
            options.stage == "all" && willRunMultiview && meshMemory.hasMesh();
        const int code = runTimedStage("model", [&] {
            if (skipModelForMemoryMultiview) {
                skippedModelForMemoryMultiview = true;
                std::cout << "[model] skipped: multiview will consume memory mesh directly." << std::endl;
                return 0;
            }
            return runModelStage(config, options, meshMemory.hasMesh() ? &meshMemory : nullptr);
        }, timings);
        if (!skipModelForMemoryMultiview) {
            meshMemory.clear();
        }
        if (code != 0) return finish(code);
    }

    if (willRunMultiview) {
        // Direct atlas scatter is available, but the current strided memory writes are slower
        // than the threaded elemental transform for the 270x270 view set.
        const bool directAtlasElemental = false;
        const int code = runTimedStage("multiview", [&] {
            return runMultiviewStage(
                config,
                options,
                &multiviewMemory,
                directAtlasElemental ? &elementalMemory : nullptr,
                meshMemory.hasMesh() ? &meshMemory : nullptr);
        }, timings);
        meshMemory.clear();
        if (code != 0) return finish(code);
    }

    if (shouldRunStage(options, "elemental") && config.runElemental) {
        const int code = runTimedStage("elemental", [&] {
            if (elementalMemory.hasResult()) {
                std::cout << "[elemental] output already materialized by direct atlas path." << std::endl;
                return 0;
            }
            return runElementalStage(
                config,
                options,
                multiviewMemory.sink ? &multiviewMemory : nullptr,
                &elementalMemory);
        }, timings);
        if (code != 0) return finish(code);
    }

    return finish(0);
}

} // namespace

int runHoloPipelineCli(int argc, char* argv[])
{
    return runHoloPipelineCliWithResult(argc, argv, nullptr);
}

int runHoloPipelineCliWithResult(int argc, char* argv[], ElementalMemoryResult* elementalResult)
{
    const CliOptions options = parseCli(argc, argv);
    if (options.showHelp) {
        printUsage();
        return 0;
    }

    fs::path configPath = options.configPath;
    if (configPath.empty()) {
        configPath = "holo_config.ini";
    }

    if (!fs::exists(configPath)) {
        printUsage();
        std::cerr << "\n[error] Config file not found: " << configPath.string() << std::endl;
        return 1;
    }

    try {
        HoloConfig config;
        applyConfig(config, configPath);
        return runPipeline(config, options, elementalResult);
    }
    catch (const std::exception& ex) {
        std::cerr << "[error] " << ex.what() << std::endl;
        return 1;
    }
}

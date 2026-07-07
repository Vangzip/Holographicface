#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "ElementalStage.h"
#include "ConverPointCloud.h"
#include "FileLibrary.h"
#include "PipelineConfig.h"
#include "PipelineData.h"
#include "PipelineLogger.h"
#include "PipelineTiming.h"
#include "depth_io.h"
#include "memoryFrameSink.h"
#include "multiviewAtlasPlan.h"
#include "multiviewAtlasRenderer.h"
#include "multiviewGraphicsConfig.h"
#include "multiviewRenderPlan.h"
#include "modelMoveHandler.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

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

int decimalDigits(int value) {
    int digits = 1;
    while (value >= 10) {
        value /= 10;
        ++digits;
    }
    return digits;
}

fs::path viewPath(const HoloConfig& config, int row, int col) {
    const std::string name = padNumber(row, config.viewNameDigits)
        + padNumber(col, config.viewNameDigits)
        + ".jpg";
    return config.multiviewOutDir / name;
}

fs::path targetPath(const HoloConfig& config, int row, int col) {
    const int rowDigits = decimalDigits(config.targetRows);
    const int colDigits = decimalDigits(config.targetCols);
    const std::string name = padNumber(row, rowDigits) + padNumber(col, colDigits) + ".jpg";
    return config.elementalOutDir / name;
}

fs::path findFirstObj(const fs::path& directory) {
    if (!fs::exists(directory)) {
        return {};
    }

    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (lower(entry.path().extension().string()) == ".obj") {
            return entry.path();
        }
    }

    return {};
}

bool shouldRunStage(const CliOptions& options, const std::string& stageName) {
    return options.stage == "all" || options.stage == stageName;
}

fs::path meshOutputPath(const HoloConfig& config, const fs::path& inputPath) {
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

int runDepthStage(const HoloConfig& config, const CliOptions& options) {
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

int runSingleMeshInput(
    const HoloConfig& config,
    const fs::path& inputPath,
    bool dryRun,
    const std::string& label) {
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
    return 0;
}

int runMeshOneStage(const HoloConfig& config, const CliOptions& options) {
    if (options.inputPath.empty()) {
        std::cerr << "[mesh-one] --input is required." << std::endl;
        return 1;
    }

    return runSingleMeshInput(config, options.inputPath, options.dryRun, "mesh-one");
}

int runMeshStage(const HoloConfig& config, const CliOptions& options) {
    if (!requireExists(config.depthInputDir, "depth_input_dir")
        || !requireExists(config.meshConfig, "mesh_config")) {
        return 1;
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

    return runSingleMeshInput(config, inputPath, options.dryRun, "mesh");
}

int runModelStage(const HoloConfig& config, const CliOptions& options) {
    if (!requireExists(config.depthInputDir, "depth_input_dir")
        || !requireExists(config.meshConfig, "mesh_config")) {
        return 1;
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

bool setMasterViewerGraphicsContext(
    osgViewer::Viewer* viewer,
    float x,
    float y,
    int width,
    int height,
    const MultiviewGraphicsConfig& graphicsConfig) {
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
    traits->x = x;
    traits->y = y;
    traits->width = width;
    traits->height = height;
    traits->windowDecoration = graphicsConfig.windowDecoration;
    traits->doubleBuffer = graphicsConfig.doubleBuffer;
    traits->vsync = graphicsConfig.vsync;
    traits->pbuffer = graphicsConfig.pbuffer;
    traits->sharedContext = 0;
    traits->alpha = 1;

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    if (!gc.valid()) {
        std::cerr << "[multiview] graphics context was not created." << std::endl;
        return false;
    }

    double fovy, aspectRatio, zNear, zFar;
    viewer->getCamera()->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);
    const double newAspectRatio = static_cast<double>(traits->width) / static_cast<double>(traits->height);
    const double aspectRatioChange = newAspectRatio / aspectRatio;
    if (aspectRatioChange != 1.0) {
        viewer->getCamera()->getProjectionMatrix() *= osg::Matrix::scale(1.0 / aspectRatioChange, 1.0, 1.0);
    }

    viewer->getCamera()->setViewport(new osg::Viewport(0, 0, width, height));
    viewer->getCamera()->setDrawBuffer(graphicsConfig.drawBuffer == MultiviewDrawBufferFront ? GL_FRONT : GL_BACK);
    viewer->getCamera()->setReadBuffer(graphicsConfig.readBuffer == MultiviewDrawBufferFront ? GL_FRONT : GL_BACK);
    viewer->getCamera()->setGraphicsContext(gc);
    return true;
}

int runMultiviewStage(HoloConfig& config, const CliOptions& options, MultiviewMemoryResult* memoryResult) {
    if (config.meshObj.empty()) {
        config.meshObj = findFirstObj(config.depthInputDir);
    }

    const int expectedViews = config.multiviewAngle * config.multiviewPer;
    if (expectedViews <= 0 || config.multiviewResolution <= 0) {
        std::cerr << "[multiview] multiview_angle, multiview_per, and multiview_resolution must be positive." << std::endl;
        return 1;
    }

    if (options.dryRun) {
        std::cout << "[multiview] render " << config.meshObj.string()
                  << " to memory as " << expectedViews << "x" << expectedViews
                  << " views, " << config.multiviewResolution << "x"
                  << config.multiviewResolution << " each" << std::endl;
        std::cout << "[multiview] atlas max texture: " << config.multiviewAtlasSize << std::endl;
        std::cout << "[multiview] camera distance scale: " << config.multiviewCamera.distanceScale
                  << ", center offset: ("
                  << config.multiviewCamera.centerOffset.x() << ", "
                  << config.multiviewCamera.centerOffset.y() << ", "
                  << config.multiviewCamera.centerOffset.z() << ")"
                  << ", eye dir: ("
                  << config.multiviewCamera.eyeDirection.x() << ", "
                  << config.multiviewCamera.eyeDirection.y() << ", "
                  << config.multiviewCamera.eyeDirection.z() << ")"
                  << ", up: ("
                  << config.multiviewCamera.upDirection.x() << ", "
                  << config.multiviewCamera.upDirection.y() << ", "
                  << config.multiviewCamera.upDirection.z() << ")" << std::endl;
        if (config.multiviewCamera.fovyDeg > 0.0) {
            std::cout << "[multiview] camera fovy: " << config.multiviewCamera.fovyDeg
                      << " deg, zNear: " << config.multiviewCamera.zNear
                      << ", zFar: " << config.multiviewCamera.zFar << std::endl;
        }
        if (!config.meshObj.empty() && !fs::exists(config.meshObj)) {
            std::cout << "[multiview] mesh_obj does not exist yet; it should be created by the model stage." << std::endl;
        }
        return 0;
    }

    if (!requireExists(config.meshObj, "mesh_obj")) {
        return 1;
    }

    if (config.modelType != "obj") {
        std::cerr << "[multiview] integrated renderer currently supports model_type=obj." << std::endl;
        return 1;
    }

    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setCameraManipulator(nullptr);
    viewer->getCamera()->setClearColor(osg::Vec4f(0.3f, 0.3f, 0.3f, 1.0f));

    MultiviewRenderPlan renderPlan(config.multiviewAngle, config.multiviewPer, config.multiviewResolution);
    MultiviewAtlasPlan atlasPlan(renderPlan, config.multiviewAtlasSize);
    const MultiviewGraphicsConfig graphicsConfig = makeMultiviewGraphicsConfig(true);
    if (!setMasterViewerGraphicsContext(
            viewer.get(),
            100,
            100,
            atlasPlan.pageWidth(),
            atlasPlan.pageHeight(),
            graphicsConfig)) {
        return 1;
    }

    osg::StateSet* state = viewer->getCamera()->getOrCreateStateSet();
    state->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);

    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(config.meshObj.string());
    if (!node.valid()) {
        std::cerr << "[multiview] cannot read OBJ: " << config.meshObj.string() << std::endl;
        return 1;
    }

    osg::ref_ptr<osg::Group> group = new osg::Group;
    group->addChild(node.get());

    std::string outDir = config.multiviewOutDir.string();
    modelMoveHandler* handler = new modelMoveHandler(
        viewer.get(), group.get(), outDir, nullptr, config.modelType,
        static_cast<float>(config.multiviewAngle), static_cast<float>(config.multiviewPer),
        config.multiviewCamera);

    try {
        std::unique_ptr<MemoryFrameSink> sink(new MemoryFrameSink(renderPlan, true));
        MultiviewAtlasRenderer renderer(viewer.get(), handler->modelTransform(), renderPlan, atlasPlan, sink.get());
        const MultiviewAtlasStats stats = renderer.renderAll();

        std::cout << "[multiview] output mode: atlas-memory" << std::endl;
        std::cout << "[multiview] pbuffer: " << graphicsConfig.pbuffer
                  << ", double buffer: " << graphicsConfig.doubleBuffer
                  << ", vsync: " << graphicsConfig.vsync
                  << ", window decoration: " << graphicsConfig.windowDecoration << std::endl;
        std::cout << "[multiview] atlas page: " << atlasPlan.pageWidth()
                  << "x" << atlasPlan.pageHeight()
                  << ", tiles per axis: " << atlasPlan.tilesPerAxis()
                  << ", pages: " << atlasPlan.pageCount() << std::endl;
        std::cout << "[multiview] frames captured: " << stats.framesCaptured
                  << "/" << renderPlan.frameCount()
                  << ", bytes: " << stats.bytesCaptured
                  << ", readback errors: " << stats.readbackErrors << std::endl;
        std::cout << "[multiview] render: " << formatSeconds(stats.renderSeconds)
                  << "s, readback: " << formatSeconds(stats.readbackSeconds)
                  << "s, copy: " << formatSeconds(stats.copySeconds)
                  << "s, total: " << formatSeconds(stats.totalSeconds) << "s" << std::endl;

        if (stats.pagesRendered != atlasPlan.pageCount()
            || stats.pageReadbacks != atlasPlan.pageCount()
            || stats.framesCaptured != renderPlan.frameCount()
            || stats.bytesCaptured != renderPlan.totalBytes()
            || stats.readbackErrors != 0) {
            std::cerr << "[multiview] memory render did not capture all frames." << std::endl;
            return 1;
        }

        if (memoryResult != nullptr) {
            memoryResult->plan.reset(new MultiviewRenderPlan(renderPlan));
            memoryResult->sink = std::move(sink);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "[multiview] memory render failed: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}

int runPipeline(HoloConfig& config, const CliOptions& options) {
    const auto pipelineStart = std::chrono::steady_clock::now();
    std::vector<StageTiming> timings;
    MultiviewMemoryResult multiviewMemory;
    ElementalMemoryResult elementalMemory;
    auto finish = [&](int code) {
        printTimingSummary(timings);
        const double wallSeconds = elapsedSeconds(pipelineStart);
        writePipelineLog(config, timings, multiviewMemory, elementalMemory, code, wallSeconds);
        std::cout << "[log] pipeline log: " << config.logFile.string() << std::endl;
        return code;
    };

    if (options.stage == "mesh-one") {
        const int code = runTimedStage("mesh-one", [&] { return runMeshOneStage(config, options); }, timings);
        return finish(code);
    }

    if (shouldRunStage(options, "depth") && config.runDepthPointCloud) {
        const int code = runTimedStage("depth", [&] { return runDepthStage(config, options); }, timings);
        if (code != 0) return finish(code);
    }

    if (shouldRunStage(options, "mesh") && config.runMesh) {
        const int code = runTimedStage("mesh", [&] { return runMeshStage(config, options); }, timings);
        if (code != 0) return finish(code);
    }

    if (shouldRunStage(options, "model") && config.runTexturedModel) {
        const int code = runTimedStage("model", [&] { return runModelStage(config, options); }, timings);
        if (code != 0) return finish(code);
    }

    if (shouldRunStage(options, "multiview") && config.runMultiview) {
        const bool keepForElemental = shouldRunStage(options, "elemental") && config.runElemental;
        const int code = runTimedStage("multiview", [&] {
            return runMultiviewStage(config, options, keepForElemental ? &multiviewMemory : nullptr);
        }, timings);
        if (code != 0) return finish(code);
    }

    if (shouldRunStage(options, "elemental") && config.runElemental) {
        const int code = runTimedStage("elemental", [&] {
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

int runHoloPipelineCli(int argc, char* argv[]) {
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
        return runPipeline(config, options);
    }
    catch (const std::exception& ex) {
        std::cerr << "[error] " << ex.what() << std::endl;
        return 1;
    }
}

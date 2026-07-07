#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "MultiviewStage.h"

#include "memoryFrameSink.h"
#include "multiviewAtlasPlan.h"
#include "multiviewAtlasRenderer.h"
#include "multiviewGraphicsConfig.h"
#include "multiviewRenderPlan.h"
#include "modelMoveHandler.h"
#include "PipelineTiming.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
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

fs::path findFirstObj(const fs::path& directory)
{
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

bool setMasterViewerGraphicsContext(
    osgViewer::Viewer* viewer,
    float x,
    float y,
    int width,
    int height,
    const MultiviewGraphicsConfig& graphicsConfig)
{
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

} // namespace

int runMultiviewStage(HoloConfig& config, const CliOptions& options, MultiviewMemoryResult* memoryResult)
{
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

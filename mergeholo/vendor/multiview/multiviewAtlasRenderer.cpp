#include "multiviewAtlasRenderer.h"
#include "multiviewCameraOrbit.h"

#include <osg/Scissor>

#include <chrono>
#include <cmath>
#include <stdexcept>

namespace {
double secondsBetween(std::chrono::high_resolution_clock::time_point start,
                      std::chrono::high_resolution_clock::time_point end) {
    return std::chrono::duration<double>(end - start).count();
}

osg::Matrixd orbitViewMatrix(const osg::Vec3d& center,
                             const osg::Vec3d& eyeDirection,
                             const osg::Vec3d& up,
                             const osg::Vec3d& right,
                             double distance,
                             const MultiviewOrbitAngles& angles)
{
    const double yaw = osg::DegreesToRadians(angles.yawDegrees);
    const double pitch = osg::DegreesToRadians(angles.pitchDegrees);
    const osg::Vec3d horizontal = eyeDirection * std::cos(yaw) + right * std::sin(yaw);
    const osg::Vec3d offset = horizontal * std::cos(pitch) + up * std::sin(pitch);
    return osg::Matrixd::lookAt(center + offset * distance, center, up);
}

class AtlasCaptureDrawCallback : public osg::Camera::DrawCallback {
public:
    AtlasCaptureDrawCallback(const MultiviewAtlasPlan& atlasPlan, MemoryAtlasPageSink* sink)
        : atlasPlan_(atlasPlan),
          sink_(sink),
          pageIndex_(0),
          pageReadbacks_(0),
          framesCaptured_(0),
          readbackErrors_(0),
          readbackSeconds_(0.0),
          copySeconds_(0.0) {}

    void setPage(std::uint64_t pageIndex) const {
        pageIndex_ = pageIndex;
    }

    std::uint64_t pageReadbacks() const {
        return pageReadbacks_;
    }

    std::uint64_t framesCaptured() const {
        return framesCaptured_;
    }

    unsigned int readbackErrors() const {
        return readbackErrors_;
    }

    double readbackSeconds() const {
        return readbackSeconds_;
    }

    double copySeconds() const {
        return copySeconds_;
    }

    virtual void operator()(const osg::Camera& camera) const {
        while (glGetError() != GL_NO_ERROR) {
        }

        GLint previousPackAlignment = 4;
        GLint previousPackRowLength = 0;
        GLint previousPackSkipRows = 0;
        GLint previousPackSkipPixels = 0;
        glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
        glGetIntegerv(GL_PACK_ROW_LENGTH, &previousPackRowLength);
        glGetIntegerv(GL_PACK_SKIP_ROWS, &previousPackSkipRows);
        glGetIntegerv(GL_PACK_SKIP_PIXELS, &previousPackSkipPixels);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_PACK_ROW_LENGTH, 0);
        glPixelStorei(GL_PACK_SKIP_ROWS, 0);
        glPixelStorei(GL_PACK_SKIP_PIXELS, 0);

        const auto restorePackState = [&]() {
            glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
            glPixelStorei(GL_PACK_ROW_LENGTH, previousPackRowLength);
            glPixelStorei(GL_PACK_SKIP_ROWS, previousPackSkipRows);
            glPixelStorei(GL_PACK_SKIP_PIXELS, previousPackSkipPixels);
        };

        const auto readStart = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::time_point readEnd;
        try {
            unsigned char* pageBuffer = sink_->pageData(pageIndex_);
            glReadPixels(0,
                         0,
                         atlasPlan_.pageWidth(),
                         atlasPlan_.pageHeight(),
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         pageBuffer);
            readEnd = std::chrono::high_resolution_clock::now();
        } catch (...) {
            restorePackState();
            throw;
        }
        restorePackState();

        if (glGetError() != GL_NO_ERROR) {
            ++readbackErrors_;
            readbackSeconds_ += secondsBetween(readStart, readEnd);
            return;
        }

        const auto copyStart = std::chrono::high_resolution_clock::now();
        const std::uint64_t framesOnPage = atlasPlan_.frameCountOnPage(pageIndex_);
        sink_->afterPageReadback(pageIndex_);
        const auto copyEnd = std::chrono::high_resolution_clock::now();

        ++pageReadbacks_;
        framesCaptured_ += framesOnPage;
        readbackSeconds_ += secondsBetween(readStart, readEnd);
        copySeconds_ += secondsBetween(copyStart, copyEnd);
    }

private:
    const MultiviewAtlasPlan& atlasPlan_;
    MemoryAtlasPageSink* sink_;
    mutable std::uint64_t pageIndex_;
    mutable std::uint64_t pageReadbacks_;
    mutable std::uint64_t framesCaptured_;
    mutable unsigned int readbackErrors_;
    mutable double readbackSeconds_;
    mutable double copySeconds_;
};
}

MultiviewAtlasRenderer::MultiviewAtlasRenderer(osgViewer::Viewer* viewer,
                                               osg::MatrixTransform* modelTransform,
                                               const MultiviewRenderPlan& renderPlan,
                                               const MultiviewAtlasPlan& atlasPlan,
                                               MemoryAtlasPageSink* sink)
    : viewer_(viewer),
      modelTransform_(modelTransform),
      renderPlan_(renderPlan),
      atlasPlan_(atlasPlan),
      sink_(sink) {
    if (viewer_ == nullptr) {
        throw std::invalid_argument("viewer must not be null");
    }
    if (modelTransform_ == nullptr) {
        throw std::invalid_argument("modelTransform must not be null");
    }
    if (sink_ == nullptr) {
        throw std::invalid_argument("sink must not be null");
    }
}

MultiviewAtlasStats MultiviewAtlasRenderer::renderAll() {
    MultiviewAtlasStats stats = {};
    const auto totalStart = std::chrono::high_resolution_clock::now();

    if (!viewer_->isRealized()) {
        viewer_->realize();
    }

    buildFrameViewMatrices();

    osg::ref_ptr<osg::Node> originalScene = viewer_->getSceneData();
    osg::ref_ptr<osg::Camera::DrawCallback> originalPostDrawCallback =
        viewer_->getCamera()->getPostDrawCallback();
    osg::ref_ptr<AtlasCaptureDrawCallback> captureCallback =
        new AtlasCaptureDrawCallback(atlasPlan_, sink_);
    viewer_->getCamera()->setPostDrawCallback(captureCallback.get());

    const auto restoreViewerState = [&]() {
        viewer_->getCamera()->setPostDrawCallback(originalPostDrawCallback.get());
        viewer_->setSceneData(originalScene.get());
    };

    try {
        for (std::uint64_t pageIndex = 0; pageIndex < atlasPlan_.pageCount(); ++pageIndex) {
            osg::ref_ptr<osg::Group> pageScene = createPageScene(pageIndex);
            captureCallback->setPage(pageIndex);
            viewer_->setSceneData(pageScene.get());

            const auto renderStart = std::chrono::high_resolution_clock::now();
            viewer_->frame();
            const auto renderEnd = std::chrono::high_resolution_clock::now();
            stats.renderSeconds += secondsBetween(renderStart, renderEnd);
            ++stats.pagesRendered;
        }
    } catch (...) {
        restoreViewerState();
        throw;
    }

    restoreViewerState();

    const auto totalEnd = std::chrono::high_resolution_clock::now();
    stats.pageReadbacks = captureCallback->pageReadbacks();
    stats.framesCaptured = captureCallback->framesCaptured();
    stats.readbackErrors = captureCallback->readbackErrors();
    stats.readbackSeconds = captureCallback->readbackSeconds();
    stats.copySeconds = captureCallback->copySeconds();
    stats.bytesCaptured =
        stats.readbackErrors == 0 && stats.framesCaptured == renderPlan_.frameCount()
            ? sink_->capturedBytes(stats.pageReadbacks)
            : 0;
    stats.renderSeconds -= stats.readbackSeconds;
    if (stats.renderSeconds < 0.0) {
        stats.renderSeconds = 0.0;
    }
    stats.totalSeconds = secondsBetween(totalStart, totalEnd);
    return stats;
}

void MultiviewAtlasRenderer::buildFrameViewMatrices() {
    frameViewMatrices_.clear();
    frameViewMatrices_.reserve(static_cast<std::size_t>(renderPlan_.frameCount()));

    osg::Vec3d eye;
    osg::Vec3d viewCenter;
    osg::Vec3d up;
    viewer_->getCamera()->getViewMatrixAsLookAt(eye, viewCenter, up);
    const osg::Vec3d orbitCenter = viewCenter;
    osg::Vec3d eyeDirection = eye - orbitCenter;
    const double distance = eyeDirection.length();
    if (distance <= 0.000001 || up.normalize() <= 0.000001) {
        throw std::runtime_error("invalid multiview camera basis");
    }
    eyeDirection /= distance;
    osg::Vec3d right = up ^ eyeDirection;
    if (right.normalize() <= 0.000001) {
        throw std::runtime_error("multiview camera up is parallel to its eye direction");
    }
    up = eyeDirection ^ right;
    up.normalize();

    for (int row = 0; row < renderPlan_.samplesPerAxis(); ++row) {
        for (int column = 0; column < renderPlan_.samplesPerAxis(); ++column) {
            const MultiviewOrbitAngles angles = multiviewOrbitAngles(
                renderPlan_.angle(),
                renderPlan_.samplesPerAxis(),
                renderPlan_.stepDegrees(),
                row,
                column);
            frameViewMatrices_.push_back(
                orbitViewMatrix(orbitCenter, eyeDirection, up, right, distance, angles));
        }
    }
}

osg::Camera* MultiviewAtlasRenderer::createTileCamera(
    const MultiviewAtlasTile& tile,
    const osg::Matrixd& modelMatrix,
    const osg::Matrixd& viewMatrix) const {
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setRenderOrder(osg::Camera::NESTED_RENDER);
    camera->setViewport(new osg::Viewport(tile.x,
                                          tile.y,
                                          atlasPlan_.tileSize(),
                                          atlasPlan_.tileSize()));
    camera->setViewMatrix(viewMatrix);
    camera->setProjectionMatrix(viewer_->getCamera()->getProjectionMatrix());
    camera->setClearColor(viewer_->getCamera()->getClearColor());
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);
    camera->getOrCreateStateSet()->setAttributeAndModes(
        new osg::Scissor(tile.x, tile.y, atlasPlan_.tileSize(), atlasPlan_.tileSize()),
        osg::StateAttribute::ON);

    osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform;
    transform->setMatrix(modelMatrix);
    for (unsigned int i = 0; i < modelTransform_->getNumChildren(); ++i) {
        transform->addChild(modelTransform_->getChild(i));
    }
    camera->addChild(transform.get());
    return camera.release();
}

osg::Group* MultiviewAtlasRenderer::createPageScene(std::uint64_t pageIndex) const {
    osg::ref_ptr<osg::Group> pageRoot = new osg::Group;
    const std::uint64_t firstFrame = atlasPlan_.firstFrameOnPage(pageIndex);
    const std::uint64_t framesOnPage = atlasPlan_.frameCountOnPage(pageIndex);

    for (std::uint64_t i = 0; i < framesOnPage; ++i) {
        const std::uint64_t frameIndex = firstFrame + i;
        const MultiviewAtlasTile tile = atlasPlan_.tileForFrame(frameIndex);
        pageRoot->addChild(createTileCamera(
            tile,
            modelTransform_->getMatrix(),
            frameViewMatrices_[static_cast<std::size_t>(frameIndex)]));
    }

    return pageRoot.release();
}

#include "multiviewAtlasRenderer.h"

#include <osg/Scissor>

#include <chrono>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {
double secondsBetween(std::chrono::high_resolution_clock::time_point start,
                      std::chrono::high_resolution_clock::time_point end) {
    return std::chrono::duration<double>(end - start).count();
}

class AtlasCaptureDrawCallback : public osg::Camera::DrawCallback {
public:
    AtlasCaptureDrawCallback(const MultiviewAtlasPlan& atlasPlan, MemoryFrameSink* sink)
        : atlasPlan_(atlasPlan),
          sink_(sink),
          pageIndex_(0),
          pageReadbacks_(0),
          framesCaptured_(0),
          readbackErrors_(0),
          readbackSeconds_(0.0),
          copySeconds_(0.0),
          pageBuffer_(static_cast<std::size_t>(atlasPlan.pageBytes())) {}

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

        const auto readStart = std::chrono::high_resolution_clock::now();
        glReadPixels(0,
                     0,
                     atlasPlan_.pageWidth(),
                     atlasPlan_.pageHeight(),
                     GL_RGB,
                     GL_UNSIGNED_BYTE,
                     pageBuffer_.data());
        const auto readEnd = std::chrono::high_resolution_clock::now();

        glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
        glPixelStorei(GL_PACK_ROW_LENGTH, previousPackRowLength);
        glPixelStorei(GL_PACK_SKIP_ROWS, previousPackSkipRows);
        glPixelStorei(GL_PACK_SKIP_PIXELS, previousPackSkipPixels);

        if (glGetError() != GL_NO_ERROR) {
            ++readbackErrors_;
            readbackSeconds_ += secondsBetween(readStart, readEnd);
            return;
        }

        const auto copyStart = std::chrono::high_resolution_clock::now();
        const std::uint64_t framesOnPage = atlasPlan_.frameCountOnPage(pageIndex_);
        const std::uint64_t firstFrame = atlasPlan_.firstFrameOnPage(pageIndex_);
        const int tileSize = atlasPlan_.tileSize();
        const int channels = 3;
        const std::size_t rowBytes = static_cast<std::size_t>(tileSize) * channels;

        for (std::uint64_t i = 0; i < framesOnPage; ++i) {
            const std::uint64_t frameIndex = firstFrame + i;
            const MultiviewAtlasTile tile = atlasPlan_.tileForFrame(frameIndex);
            unsigned char* target = sink_->frameData(frameIndex);

            for (int row = 0; row < tileSize; ++row) {
                const std::size_t sourceOffset =
                    (static_cast<std::size_t>(tile.y + row) *
                         static_cast<std::size_t>(atlasPlan_.pageWidth()) +
                     static_cast<std::size_t>(tile.x)) *
                    channels;
                std::memcpy(target + static_cast<std::size_t>(row) * rowBytes,
                            pageBuffer_.data() + sourceOffset,
                            rowBytes);
            }
        }
        const auto copyEnd = std::chrono::high_resolution_clock::now();

        ++pageReadbacks_;
        framesCaptured_ += framesOnPage;
        readbackSeconds_ += secondsBetween(readStart, readEnd);
        copySeconds_ += secondsBetween(copyStart, copyEnd);
    }

private:
    const MultiviewAtlasPlan& atlasPlan_;
    MemoryFrameSink* sink_;
    mutable std::uint64_t pageIndex_;
    mutable std::uint64_t pageReadbacks_;
    mutable std::uint64_t framesCaptured_;
    mutable unsigned int readbackErrors_;
    mutable double readbackSeconds_;
    mutable double copySeconds_;
    mutable std::vector<unsigned char> pageBuffer_;
};
}

MultiviewAtlasRenderer::MultiviewAtlasRenderer(osgViewer::Viewer* viewer,
                                               osg::MatrixTransform* modelTransform,
                                               const MultiviewRenderPlan& renderPlan,
                                               const MultiviewAtlasPlan& atlasPlan,
                                               MemoryFrameSink* sink)
    : viewer_(viewer),
      modelTransform_(modelTransform),
      renderPlan_(renderPlan),
      atlasPlan_(atlasPlan),
      sink_(sink),
      rotationCenter_() {
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

    rotationCenter_ = modelTransform_->getBound().center();
    buildFrameMatrices();

    osg::ref_ptr<osg::Node> originalScene = viewer_->getSceneData();
    osg::ref_ptr<AtlasCaptureDrawCallback> captureCallback =
        new AtlasCaptureDrawCallback(atlasPlan_, sink_);
    viewer_->getCamera()->setPostDrawCallback(captureCallback.get());

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

    viewer_->getCamera()->setPostDrawCallback(NULL);
    viewer_->setSceneData(originalScene.get());

    const auto totalEnd = std::chrono::high_resolution_clock::now();
    stats.pageReadbacks = captureCallback->pageReadbacks();
    stats.framesCaptured = captureCallback->framesCaptured();
    stats.readbackErrors = captureCallback->readbackErrors();
    stats.readbackSeconds = captureCallback->readbackSeconds();
    stats.copySeconds = captureCallback->copySeconds();
    stats.bytesCaptured =
        stats.readbackErrors == 0 && stats.framesCaptured == renderPlan_.frameCount()
            ? renderPlan_.totalBytes()
            : 0;
    stats.renderSeconds -= stats.readbackSeconds;
    if (stats.renderSeconds < 0.0) {
        stats.renderSeconds = 0.0;
    }
    stats.totalSeconds = secondsBetween(totalStart, totalEnd);
    return stats;
}

osg::Matrixd MultiviewAtlasRenderer::rotateZ(const osg::Matrixd& matrix,
                                             double degrees) const {
    return matrix *
           osg::Matrixd::translate(-rotationCenter_) *
           osg::Matrixd::rotate(-osg::DegreesToRadians(degrees), 0, 0, 1) *
           osg::Matrixd::translate(rotationCenter_);
}

osg::Matrixd MultiviewAtlasRenderer::rotateX(const osg::Matrixd& matrix,
                                             double degrees) const {
    return matrix *
           osg::Matrixd::translate(-rotationCenter_) *
           osg::Matrixd::rotate(osg::DegreesToRadians(degrees), 1, 0, 0) *
           osg::Matrixd::translate(rotationCenter_);
}

void MultiviewAtlasRenderer::buildFrameMatrices() {
    frameMatrices_.clear();
    frameMatrices_.reserve(static_cast<std::size_t>(renderPlan_.frameCount()));

    osg::Matrixd matrix = modelTransform_->getMatrix();
    for (int row = 0; row < renderPlan_.samplesPerAxis(); ++row) {
        if (row > 0) {
            matrix = rotateZ(matrix, -static_cast<double>(renderPlan_.angle()) / 2.0);
            matrix = rotateX(matrix, -renderPlan_.stepDegrees());
            matrix = rotateZ(matrix, -static_cast<double>(renderPlan_.angle()) / 2.0);
        }

        for (int column = 0; column < renderPlan_.samplesPerAxis(); ++column) {
            matrix = rotateZ(matrix, renderPlan_.stepDegrees());
            frameMatrices_.push_back(matrix);
        }
    }
}

osg::Camera* MultiviewAtlasRenderer::createTileCamera(
    const MultiviewAtlasTile& tile,
    const osg::Matrixd& modelMatrix) const {
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setRenderOrder(osg::Camera::NESTED_RENDER);
    camera->setViewport(new osg::Viewport(tile.x,
                                          tile.y,
                                          atlasPlan_.tileSize(),
                                          atlasPlan_.tileSize()));
    camera->setViewMatrix(viewer_->getCamera()->getViewMatrix());
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
        pageRoot->addChild(createTileCamera(tile, frameMatrices_[static_cast<std::size_t>(frameIndex)]));
    }

    return pageRoot.release();
}

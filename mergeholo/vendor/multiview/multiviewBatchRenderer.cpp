#include "multiviewBatchRenderer.h"
#include "multiviewOrbitMatrices.h"

#include <chrono>
#include <cmath>
#include <stdexcept>

namespace {
double secondsBetween(std::chrono::high_resolution_clock::time_point start,
                      std::chrono::high_resolution_clock::time_point end) {
    return std::chrono::duration<double>(end - start).count();
}

class MemoryCaptureDrawCallback : public osg::Camera::DrawCallback {
public:
    explicit MemoryCaptureDrawCallback(int resolution)
        : resolution_(resolution),
          target_(nullptr),
          framesCaptured_(0),
          readbackErrors_(0),
          readbackSeconds_(0.0) {}

    void setTarget(unsigned char* target) const {
        target_ = target;
    }

    double readbackSeconds() const {
        return readbackSeconds_;
    }

    std::uint64_t framesCaptured() const {
        return framesCaptured_;
    }

    unsigned int readbackErrors() const {
        return readbackErrors_;
    }

    virtual void operator()(const osg::Camera& camera) const {
        if (target_ == nullptr) {
            return;
        }

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

        const auto start = std::chrono::high_resolution_clock::now();
        glReadPixels(0,
                     0,
                     resolution_,
                     resolution_,
                     GL_RGB,
                     GL_UNSIGNED_BYTE,
                     target_);
        const auto end = std::chrono::high_resolution_clock::now();
        glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
        glPixelStorei(GL_PACK_ROW_LENGTH, previousPackRowLength);
        glPixelStorei(GL_PACK_SKIP_ROWS, previousPackSkipRows);
        glPixelStorei(GL_PACK_SKIP_PIXELS, previousPackSkipPixels);

        if (glGetError() != GL_NO_ERROR) {
            ++readbackErrors_;
        } else {
            ++framesCaptured_;
        }
        readbackSeconds_ += secondsBetween(start, end);
    }

private:
    int resolution_;
    mutable unsigned char* target_;
    mutable std::uint64_t framesCaptured_;
    mutable unsigned int readbackErrors_;
    mutable double readbackSeconds_;
};
}

MultiviewBatchRenderer::MultiviewBatchRenderer(osgViewer::Viewer* viewer,
                                               osg::MatrixTransform* modelTransform,
                                               const MultiviewRenderPlan& plan,
                                               MemoryFrameSink* sink)
    : viewer_(viewer),
      modelTransform_(modelTransform),
      plan_(plan),
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

MultiviewBatchStats MultiviewBatchRenderer::renderAll() {
    MultiviewBatchStats stats = {};
    const auto totalStart = std::chrono::high_resolution_clock::now();

    if (!viewer_->isRealized()) {
        viewer_->realize();
    }
    const osg::Matrixd originalViewMatrix = viewer_->getCamera()->getViewMatrix();
    osg::Vec3d eye;
    osg::Vec3d viewCenter;
    osg::Vec3d up;
    viewer_->getCamera()->getViewMatrixAsLookAt(eye, viewCenter, up);
    const std::vector<osg::Matrixd> frameViewMatrices =
        buildMultiviewOrbitMatrices(eye, viewCenter, up, plan_);

    osg::ref_ptr<MemoryCaptureDrawCallback> captureCallback =
        new MemoryCaptureDrawCallback(plan_.resolution());
    osg::ref_ptr<osg::Camera::DrawCallback> originalPostDrawCallback =
        viewer_->getCamera()->getPostDrawCallback();
    viewer_->getCamera()->setPostDrawCallback(captureCallback.get());

    std::uint64_t frameIndex = 0;
    const auto restoreViewerState = [&]() {
        captureCallback->setTarget(nullptr);
        viewer_->getCamera()->setPostDrawCallback(originalPostDrawCallback.get());
        viewer_->getCamera()->setViewMatrix(originalViewMatrix);
    };

    try {
        for (int row = 0; row < plan_.samplesPerAxis(); ++row) {
            for (int column = 0; column < plan_.samplesPerAxis(); ++column) {
                viewer_->getCamera()->setViewMatrix(frameViewMatrices.at(
                    static_cast<std::size_t>(frameIndex)));
                captureCallback->setTarget(sink_->frameData(frameIndex));

                const auto renderStart = std::chrono::high_resolution_clock::now();
                viewer_->frame();
                const auto renderEnd = std::chrono::high_resolution_clock::now();

                stats.renderSeconds += secondsBetween(renderStart, renderEnd);
                ++frameIndex;
            }
        }
    } catch (...) {
        restoreViewerState();
        throw;
    }

    const auto totalEnd = std::chrono::high_resolution_clock::now();
    stats.framesRendered = frameIndex;
    stats.framesCaptured = captureCallback->framesCaptured();
    stats.readbackErrors = captureCallback->readbackErrors();
    stats.bytesCaptured =
        stats.readbackErrors == 0 && stats.framesCaptured == plan_.frameCount()
            ? plan_.totalBytes()
            : 0;
    stats.readbackSeconds = captureCallback->readbackSeconds();
    stats.renderSeconds -= stats.readbackSeconds;
    if (stats.renderSeconds < 0.0) {
        stats.renderSeconds = 0.0;
    }
    stats.totalSeconds = secondsBetween(totalStart, totalEnd);
    restoreViewerState();
    return stats;
}

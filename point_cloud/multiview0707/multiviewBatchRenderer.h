#ifndef MULTIVIEW_BATCH_RENDERER_H
#define MULTIVIEW_BATCH_RENDERER_H

#include "memoryFrameSink.h"
#include "multiviewRenderPlan.h"

#include <base.h>

#include <cstdint>

struct MultiviewBatchStats {
    std::uint64_t framesRendered;
    std::uint64_t framesCaptured;
    std::uint64_t bytesCaptured;
    unsigned int readbackErrors;
    double renderSeconds;
    double readbackSeconds;
    double totalSeconds;
};

class MultiviewBatchRenderer {
public:
    MultiviewBatchRenderer(osgViewer::Viewer* viewer,
                           osg::MatrixTransform* modelTransform,
                           const MultiviewRenderPlan& plan,
                           MemoryFrameSink* sink);

    MultiviewBatchStats renderAll();

private:
    void rotateZ(double degrees);
    void rotateX(double degrees);
    void advanceRow();

    osgViewer::Viewer* viewer_;
    osg::MatrixTransform* modelTransform_;
    osg::Vec3d rotationCenter_;
    MultiviewRenderPlan plan_;
    MemoryFrameSink* sink_;
};

#endif

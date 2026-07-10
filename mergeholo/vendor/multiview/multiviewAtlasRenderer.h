#ifndef MULTIVIEW_ATLAS_RENDERER_H
#define MULTIVIEW_ATLAS_RENDERER_H

#include "memoryAtlasPageSink.h"
#include "multiviewAtlasPlan.h"
#include "multiviewRenderPlan.h"

#include <base.h>

#include <cstdint>
#include <vector>

struct MultiviewAtlasStats {
    std::uint64_t pagesRendered;
    std::uint64_t pageReadbacks;
    std::uint64_t framesCaptured;
    std::uint64_t bytesCaptured;
    unsigned int readbackErrors;
    double renderSeconds;
    double readbackSeconds;
    double copySeconds;
    double totalSeconds;
};

class MultiviewAtlasRenderer {
public:
    MultiviewAtlasRenderer(osgViewer::Viewer* viewer,
                           osg::MatrixTransform* modelTransform,
                           const MultiviewRenderPlan& renderPlan,
                           const MultiviewAtlasPlan& atlasPlan,
                           MemoryAtlasPageSink* sink);

    MultiviewAtlasStats renderAll();

private:
    osg::Matrixd rotateZ(const osg::Matrixd& matrix, double degrees) const;
    osg::Matrixd rotateX(const osg::Matrixd& matrix, double degrees) const;
    void buildFrameMatrices();
    osg::Camera* createTileCamera(const MultiviewAtlasTile& tile,
                                  const osg::Matrixd& modelMatrix) const;
    osg::Group* createPageScene(std::uint64_t pageIndex) const;

    osgViewer::Viewer* viewer_;
    osg::MatrixTransform* modelTransform_;
    MultiviewRenderPlan renderPlan_;
    MultiviewAtlasPlan atlasPlan_;
    MemoryAtlasPageSink* sink_;
    osg::Vec3d rotationCenter_;
    std::vector<osg::Matrixd> frameMatrices_;
};

#endif

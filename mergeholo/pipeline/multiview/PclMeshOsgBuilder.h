#pragma once

#include <pcl/PolygonMesh.h>

#include <osg/Group>
#include <osg/ref_ptr>

#include <cstddef>

struct PclMeshOsgBuildResult {
    osg::ref_ptr<osg::Group> group;
    std::size_t vertexCount = 0;
    std::size_t triangleCount = 0;
    std::size_t skippedFaces = 0;
    bool usedVertexColors = false;
};

PclMeshOsgBuildResult buildOsgGroupFromPclMesh(const pcl::PolygonMesh& mesh);

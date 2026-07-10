#include "PclMeshOsgBuilder.h"

#include <pcl/PCLPointField.h>
#include <pcl/conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PrimitiveSet>
#include <osg/Vec3>
#include <osg/Vec4>

#include <cstdint>

namespace {

bool hasField(const pcl::PCLPointCloud2& cloud, const char* name)
{
    for (const pcl::PCLPointField& field : cloud.fields) {
        if (field.name == name) {
            return true;
        }
    }
    return false;
}

bool hasColorField(const pcl::PCLPointCloud2& cloud)
{
    return hasField(cloud, "rgb") || hasField(cloud, "rgba");
}

bool faceIndicesAreValid(const pcl::Vertices& face, std::size_t vertexCount)
{
    for (const std::uint32_t index : face.vertices) {
        if (index >= vertexCount) {
            return false;
        }
    }
    return true;
}

} // namespace

PclMeshOsgBuildResult buildOsgGroupFromPclMesh(const pcl::PolygonMesh& mesh)
{
    PclMeshOsgBuildResult result;

    const bool useColors = hasColorField(mesh.cloud);
    pcl::PointCloud<pcl::PointXYZ> xyzCloud;
    pcl::PointCloud<pcl::PointXYZRGB> rgbCloud;
    if (useColors) {
        pcl::fromPCLPointCloud2(mesh.cloud, rgbCloud);
    }
    else {
        pcl::fromPCLPointCloud2(mesh.cloud, xyzCloud);
    }
    const std::size_t vertexCount = useColors ? rgbCloud.size() : xyzCloud.size();

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    vertices->reserve(vertexCount);
    if (useColors) {
        colors->reserve(vertexCount);
    }

    if (useColors) {
        for (const pcl::PointXYZRGB& point : rgbCloud) {
            vertices->push_back(osg::Vec3(point.x, point.y, point.z));
            colors->push_back(osg::Vec4(
                static_cast<float>(point.r) / 255.0f,
                static_cast<float>(point.g) / 255.0f,
                static_cast<float>(point.b) / 255.0f,
                1.0f));
        }
    }
    else {
        for (const pcl::PointXYZ& point : xyzCloud) {
            vertices->push_back(osg::Vec3(point.x, point.y, point.z));
        }
    }

    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (const pcl::Vertices& face : mesh.polygons) {
        if (face.vertices.size() < 3 || !faceIndicesAreValid(face, vertexCount)) {
            ++result.skippedFaces;
            continue;
        }

        for (std::size_t i = 1; i + 1 < face.vertices.size(); ++i) {
            indices->push_back(face.vertices[0]);
            indices->push_back(face.vertices[i]);
            indices->push_back(face.vertices[i + 1]);
            ++result.triangleCount;
        }
    }

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    geometry->setVertexArray(vertices.get());
    if (useColors) {
        geometry->setColorArray(colors.get());
        geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    geometry->addPrimitiveSet(indices.get());

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geometry.get());

    result.group = new osg::Group;
    result.group->addChild(geode.get());
    result.vertexCount = vertices->size();
    result.usedVertexColors = useColors;
    return result;
}

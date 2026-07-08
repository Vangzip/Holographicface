#pragma once

#include <filesystem>
#include <string>

#include <pcl/PolygonMesh.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

struct DepthMemoryResult {
    std::string baseName;
    std::filesystem::path pointCloudPath;
    std::filesystem::path rgbPath;
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud;

    bool hasCloud() const
    {
        return cloud && !cloud->empty();
    }

    void clear()
    {
        if (cloud) {
            cloud->clear();
            cloud.reset();
        }
        baseName.clear();
        pointCloudPath.clear();
        rgbPath.clear();
    }
};

struct MeshMemoryResult {
    std::string baseName;
    std::filesystem::path meshPath;
    std::filesystem::path rgbPath;
    pcl::PolygonMesh::Ptr mesh;

    bool hasMesh() const
    {
        return mesh && !mesh->cloud.data.empty() && !mesh->polygons.empty();
    }

    void clear()
    {
        if (mesh) {
            mesh->cloud.data.clear();
            mesh->cloud.fields.clear();
            mesh->polygons.clear();
            mesh.reset();
        }
        baseName.clear();
        meshPath.clear();
        rgbPath.clear();
    }
};

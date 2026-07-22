
#pragma once

#include "OdmTexturing.hpp"
#include "base.h"
#include "FileLibrary.h"

class ConverPointCloud
{
public:
    ConverPointCloud();
    ~ConverPointCloud();

    bool meshAPI(const string &flypath, const string &, const string &outputDir = string());
    bool meshAPIFromCloud(
        const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr &cloud,
        const string &logicalFlypath,
        const string &config,
        const string &outputDir,
        pcl::PolygonMesh *meshOut = nullptr,
        bool writeMeshFile = true);

    bool modelAPI(const string &flypath, const string &);
    bool modelAPIFromMesh(
        const pcl::PolygonMesh &mesh,
        const string &logicalMeshPath,
        const string &texturePath,
        const string &config);

private:

	bool parseArguments(const string &);

	bool createGreedMesh(const string &);
    bool createGreedMeshFromCloud(
        const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr &cloud,
        const string &srcfile,
        pcl::PolygonMesh *meshOut,
        bool writeMeshFile);

	bool createModel(const string &);
    bool createModelFromMesh(const pcl::PolygonMesh &mesh, const string &logicalMeshPath);

    bool calculateVertexNormal(pcl::PointCloud<pcl::PointXYZRGBNormal> &);

    bool searchKNeightbor(const pcl::PointXYZRGB &, std::vector<int> &, float distance);

    bool fitNormal(pcl::PointCloud<pcl::PointXYZRGBNormal> &);

    bool nearestKSearchNormal(pcl::PointCloud<PointXYZRGBNormal> &);

    bool createPoissonMesh(const string &filepath);
    bool createPoissonMeshFromCloud(
        const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr &cloud,
        pcl::PolygonMesh &meshOut);

    bool normalsMovingLeastSquares(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, pcl::PointCloud<PointXYZRGBNormal> &);

    bool getPointFromPointNormal(pcl::PointCloud<pcl::PointXYZRGB>::Ptr src, pcl::PointCloud<PointXYZRGBNormal> &des);

    bool fillHole(const pcl::PolygonMesh &inPutmesh, pcl::PolygonMesh &outMesh);


    void mesh2VRML(const pcl::PolygonMesh &inPutmesh, const string &file);

    string buildMeshOutputPath(const string &srcfile, const string &suffix);


    bool getPointFromPointNormal(pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud, pcl::PointCloud<PointXYZRGBNormal> &mls_points);

private:
	//double m_bundleResizedTo, m_textureWithSize  , m_textureResolution;
    int m_kSearch, m_type, m_upsamplingType;
	float m_mu, m_searchRadius, m_distance;
	int m_nearestNeighbors, m_maxSurfaceAngle, m_minAngle, m_maxAngle, m_normalsIter1, m_normalsIter2;
    float m_mlsSearchRadius, m_holesize, m_focal_length, m_neighbor_num, m_leafsize;
    float m_upsamplingRadius, m_upsamplingStepSize, m_dilationVoxelSize, m_dilationIterations, m_pointDensity;

	string m_strOutModelPath, m_strTexturepng, m_strMeshOutputDir;

};

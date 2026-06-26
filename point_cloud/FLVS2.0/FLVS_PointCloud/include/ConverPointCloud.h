
#pragma once

#include "OdmTexturing.hpp"
#include "base.h"
#include "FileLibrary.h"

class ConverPointCloud
{
public:
    ConverPointCloud();
    ~ConverPointCloud();
	bool meshAPI(const string &flypath, const string &config);
private:

    //************************************
    // Method:    parseArguments
    // Access:    private 
    // Returns:   bool
    // Describe:  参数解析，生成mesh时使用
    // Parameter: const string & config 配置文件路径
    //************************************
    bool parseArguments(const string &config);

	//************************************
	// Method:    createGreedMesh
	// Access:    private 
	// Returns:   bool
	// Describe:  使用贪婪算法生成mesh
	// Parameter: const string & 模型文件
	//************************************
	bool createGreedMesh(const string &);

	bool createModel(const string &);

    bool calculateVertexNormal(pcl::PointCloud<pcl::PointXYZRGBNormal> &);


    //************************************
    // Method:    fitNormal
    // Access:    private 
    // Returns:   bool
    // Describe:  法线拟合功能 
    // Parameter: pcl::PointCloud<pcl::PointXYZRGBNormal> & 点云结构
    //************************************
    bool fitNormal(pcl::PointCloud<pcl::PointXYZRGBNormal> &);

    bool nearestKSearchNormal(pcl::PointCloud<PointXYZRGBNormal> &);

    //************************************
    // Method:    createPoissonMesh
    // Access:    private 
    // Returns:   bool
    // Describe:  使用泊松生成mesh
    // Parameter: const string & filepath 点云路径
    //************************************
    bool createPoissonMesh(const string &filepath);

    //bool normalsMovingLeastSquares(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, pcl::PointCloud<PointXYZRGBNormal> &);
    //bool normalsMovingLeastSquares(pcl::PointCloud<PointXYZRGBNormal>  &);

    //bool getPointFromPointNormal(pcl::PointCloud<pcl::PointXYZRGB>::Ptr src, pcl::PointCloud<PointXYZRGBNormal> &des);

    //************************************
    // Method:    fillHole
    // Access:    private 
    // Returns:   bool
    // Describe:  补洞功能
    // Parameter: const pcl::PolygonMesh & inPutmesh 输入mesh文件
    // Parameter: pcl::PolygonMesh & outMesh  输出mesh文件
    //************************************
    bool fillHole(const pcl::PolygonMesh &inPutmesh, pcl::PolygonMesh &outMesh);


    void mesh2VRML(const pcl::PolygonMesh &inPutmesh, const string &file);


    bool getPointFromPointNormal(pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud, pcl::PointCloud<PointXYZRGBNormal> &mls_points);

private:
	//double m_bundleResizedTo, m_textureWithSize  , m_textureResolution;
    int m_type, m_upsamplingType;
	float m_mu, m_searchRadius, m_distance;
	int m_nearestNeighbors, m_maxSurfaceAngle, m_minAngle, m_maxAngle, m_normalsIter1, m_normalsIter2;
    float m_mlsSearchRadius, m_holesize, m_focal_length, m_neighbor_num, m_leafsize;
    float m_upsamplingRadius, m_upsamplingStepSize, m_dilationVoxelSize, m_dilationIterations, m_pointDensity;

	string m_strOutModelPath, m_strTexturepng;

};
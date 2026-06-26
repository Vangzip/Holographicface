#pragma once

#include "base.h"
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/surface/mls.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/features/normal_3d.h>
#include <pcl/features/normal_3d_omp.h>
#include <pcl/surface/gp3.h>
#include <pcl/surface/texture_mapping.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/surface/poisson.h>
#include <pcl/filters/passthrough.h>
#include <pcl/TextureMesh.h>
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/PCLPointCloud2.h>

#include <pcl/common/transforms.h>
#include <pcl/common/centroid.h>
#include <pcl/surface/convex_hull.h>
#include <pcl/filters/crop_hull.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/filters/project_inliers.h>
#include <pcl/filters/voxel_grid.h>
#include<pcl\filters\statistical_outlier_removal.h>


#include <Eigen/Eigen> 
#include<string.h>
#include<io.h>
#include <boost/thread/thread.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
                                             //
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "imagehlp.h"
#include "FileLibrary.h"
//#pragma comment(lib,"imagehlp.lib")

using namespace cv;
using namespace std;


/*!
 * \brief   The Logger class is used to store program messages in a log file.
 * \details By using the << operator while printInCout is set, the class writes both to
 *          cout and to file, if the flag is not set, output is written to file only.
 */
class texturemeshPoisson
{
public:

    texturemeshPoisson(const std::string &, const std::string &);
    ~texturemeshPoisson();

    int meshCropHull();

    int meshCropHull(pcl::PointCloud<pcl::PointXYZRGB>::Ptr &);
    int meshCropHull(pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr &);
    

    int MLSPointCloud(const std::string &pcdFilenName);

    bool getCropHullPointIndex(std::vector<int>&v ){  v = m_crophullpointindex; };

    string getCropHullPointCouldPlyFile();

    bool getCropHullMesh( pcl::PolygonMesh &);

    //设置泊松后的mesh文件
    bool setPlygonMeshFile(const string &);
    //设置泊松后的polygonsmesh对象
    bool setPolygonMesh(const pcl::PolygonMesh &);
    
    bool getPoissonPoint(pcl::PointCloud<pcl::PointXYZRGB> &);
    bool getPoissonPoint(pcl::PointCloud<pcl::PointXYZRGBNormal> &);

    string getPolygonMeshFile(){ return m_strPolygonMeshFile; };

    bool getPolygonMesh(pcl::PolygonMesh &mesh);

    bool mergeMeshRGB(pcl::PolygonMesh &mesh, PointTRGBPtr &cloud, vector<int> index);
    bool mergeMeshRGB(pcl::PolygonMesh &mesh, PointTRGBNPtr &cloud, vector<int> index);


private:

	  string m_strDirectoryPath;

	 /** \brief mesh scale control. */
      float f_;

      /** \brief vector field */
      Eigen::Vector3f vector_field_;
      std::vector<int> m_crophullpointindex;
      std::vector<int> m_crophulloutpointindex;
      string m_strPointcloudfile;
      
      string m_strPolygonMeshFile;
      pcl::PolygonMesh m_polygonmeshptr;
      pcl::PolygonMesh m_polygonmesh;

      PointTRGBNPtr src_cloud;

};











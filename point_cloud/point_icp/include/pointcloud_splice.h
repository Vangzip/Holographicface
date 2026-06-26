
#pragma once

#include "base.h"
#include "FileLibrary.h"

class PointCloudSplice{
public:
    PointCloudSplice(const string &, const string &);
    ~PointCloudSplice(){};

    bool begineReadPlyFile();

    bool downsample(pcl::PointCloud<pcl::PointXYZRGB>::Ptr, pcl::PointCloud<pcl::PointXYZRGB>::Ptr);

    bool parseArguments(const string &path);
private:
    string m_strInplyfile1, m_strInplyfile2, outdir;
    //pcl::PointCloud<pcl::PointXYZ>::Ptr m_poiontcloud1, m_poiontcloud2, m_outpointcloud;
    double  normal_radius, feature_radius, euclidean, max_correspondence_distance, nr_iterations, voxel_grid_size1, voxel_grid_size2, min_sample_distance;
    float m_rotate, X, Y, Z;



};
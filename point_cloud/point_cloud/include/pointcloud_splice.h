
#pragma once

#include "base.h"
#include "FileLibrary.h"

class PointCloudSplice{
public:
    PointCloudSplice(const string &, const string &);
    ~PointCloudSplice(){};

    bool begineReadPlyFile();

private:
    string m_strInplyfile1, m_strInplyfile2, outdir;
    //pcl::PointCloud<pcl::PointXYZ>::Ptr m_poiontcloud1, m_poiontcloud2, m_outpointcloud;



};
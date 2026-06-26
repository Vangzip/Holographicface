
#pragma once
#include "base.h"
#include "FileLibrary.h"
//#include "SCEarthLibrary.h"

class DLL_API PCLBASE
{
public:
    PCLBASE();
    ~PCLBASE(){};

    static PCLBASE * getInstance();

    static void normalsMovingLeastSquares(PointTRGBPtr, PointTRGBN &, float radius);

    static bool PCDtoPLYconvertor(const string & input_filename, const string& output_filename);

    static bool getPointFromPointNormal(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, pcl::PointCloud<PointXYZRGBNormal> &mls_points);

    static bool nearestKSearchNormal(pcl::PointCloud<PointXYZRGBNormal> &mls_points, float, float, float);

    static bool calculateVertexNormal(pcl::PointCloud<pcl::PointXYZRGBNormal> &mls_points);
private:
    static PCLBASE* m_pInstance;
};



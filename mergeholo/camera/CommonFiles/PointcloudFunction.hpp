#ifndef POINTCLOUD_FUNTION_HPP
#define POINTCLOUD_FUNTION_HPP
#include <fstream>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include "pcl/registration/icp.h"
namespace PointcloudHelper {

static bool saveXYZ( pcl::gpu::DeviceArray<pcl::PointXYZ> pc, std::string strPath )
{
    FILE* fp = fopen(strPath.c_str(), "wb+");
    if( !fp )
    {
        return false;
    }

    std::vector<pcl::PointXYZ> vct(pc.size());
    pc.download(vct);

    for(auto pt : vct)
    {
        fwrite(&pt.x, sizeof(float),1,fp);
        fwrite(&pt.y, sizeof(float),1,fp);
        fwrite(&pt.z, sizeof(float),1,fp);
    }

    fclose(fp);
}

static void saveGpuPly( pcl::gpu::DeviceArray<pcl::PointXYZRGB> pointcloud, std::string strPath)
{

    std::ofstream fout;
    fout.open(strPath);
    if( !fout.is_open() )
    {
        return ;
    }

    if( !pointcloud.size() )
    {
        return ;
    }
    std::vector<pcl::PointXYZRGB> vct(pointcloud.size());
    pointcloud.download(vct);

    const int ciStep = 1;
    int iCnt = 0;
    for(size_t i=0; i< vct.size(); i+=ciStep )
    {
        auto& pt = vct[i];
        //iCnt ++;
        if(pt.z> -9900 && fabs(pt.z) >1)
        {
            iCnt ++;
        }
    }

    fout << "ply" << std::endl;
    fout << "format ascii 1.0" << std::endl;
    fout << "element vertex " << iCnt << std::endl;
    fout << "property float x" << std::endl;
    fout << "property float y" << std::endl;
    fout << "property float z" << std::endl;
    fout << "property float nx" << std::endl;
    fout << "property float ny" << std::endl;
    fout << "property float nz" << std::endl;
    fout << "property uchar red" << std::endl;
    fout << "property uchar green" << std::endl;
    fout << "property uchar blue" << std::endl;
    fout << "end_header" << std::endl;
    // 输出数据
    //int iCnt =0 ;
    for(size_t i=0; i< vct.size(); i+=ciStep )
    {
        auto& pt = vct[i];
        //iCnt ++;
//        if( i %10000 == 0 )
//        {
//            std::cout<<" Saving " << strPath << " as " << i <<"/" << iCnt << std::endl;
//        }


        if(pt.z> -9900 && fabs(pt.z) >1)
        {
            fout<<pt.x <<" " << pt.y <<" " << (pt.z < -9900 ? 0:pt.z)<<" 0 0 1 "
               << static_cast<unsigned short>(pt.r) <<" "
               << static_cast<unsigned short>(pt.g) <<" "
               << static_cast<unsigned short>(pt.b) << std::endl;
            //fout<<pt.x <<" " << pt.y <<" " << pt.z <<" 0 0 1 255 255 255" << std::endl;
        }

    }
    std::cout<<" Saving " << strPath << " size " << iCnt << std::endl;
    fout.close();
}
static void saveGpuPly( pcl::gpu::DeviceArray<pcl::PointXYZ> pointcloud, std::string strPath )
{

    std::ofstream fout;
    fout.open(strPath);
    if( !fout.is_open() )
    {
        return ;
    }

    if( !pointcloud.size() )
    {
        return ;
    }
    std::vector<pcl::PointXYZ> vct(pointcloud.size());
    pointcloud.download(vct);

    int iCnt = 0;


    const int ciStep = 1;

    for(size_t i=0; i< vct.size(); i+=ciStep )
    {
        auto& pt = vct[i];
        //iCnt ++;
        if(pt.z> -9900 && fabs(pt.z) >1)
        {
            iCnt ++;
        }
    }

    fout << "ply" << std::endl;
    fout << "format ascii 1.0" << std::endl;
    fout << "element vertex " << iCnt << std::endl;
    fout << "property float x" << std::endl;
    fout << "property float y" << std::endl;
    fout << "property float z" << std::endl;
    fout << "property float nx" << std::endl;
    fout << "property float ny" << std::endl;
    fout << "property float nz" << std::endl;
    fout << "property uchar red" << std::endl;
    fout << "property uchar green" << std::endl;
    fout << "property uchar blue" << std::endl;
    fout << "end_header" << std::endl;
    // 输出数据
    for(size_t i=0; i< vct.size(); i+=ciStep )
    {
        auto& pt = vct[i];
        //iCnt ++;
//        if( i %10000 == 0 &&  pt.z < -9900)
//        {
//            std::cout<<" Saving " << strPath << " as " << i <<"/" << vct.size() << std::endl;
//        }
        if(pt.z> -9900 && fabs(pt.z) >1)
        {
            fout<<pt.x <<" " << pt.y <<" " << pt.z <<" 0 0 1 255 255 255" << std::endl;
        }

    }

    std::cout<<" Saving " << strPath << " total " << vct.size() << std::endl;
    fout.close();
}

static pcl::PointCloud<pcl::PointXYZRGB>::Ptr
    getRegMatrix( pcl::PointCloud<pcl::PointXYZRGB>::Ptr p1,pcl::PointCloud<pcl::PointXYZRGB>::Ptr p2,
                          pcl::Registration<pcl::PointXYZRGB, pcl::PointXYZRGB>::Matrix4 guess)
{
    pcl::IterativeClosestPoint<pcl::PointXYZRGB, pcl::PointXYZRGB> icp;
    icp.setMaxCorrespondenceDistance(0.1);
    icp.setTransformationEpsilon(1e-10);
    icp.setEuclideanFitnessEpsilon(0.01);
    icp.setMaximumIterations (30);



    pcl::PointCloud<pcl::PointXYZRGB>::Ptr output(new pcl::PointCloud<pcl::PointXYZRGB>());

//    pcl::Registration<pcl::PointXYZRGB, pcl::PointXYZRGB>::Matrix4 guess;
//    guess <<
//             0.318003,-0.342201,0.884179,0.651296 -0.750328,
//             0.331784,0.913788,0.234332, 0.184329 -0.105771,
//             -0.888141,0.218838,0.404124,0.736096 -0.514175,
//             0,0,0,1;

    icp.setInputSource (p1);
    icp.setInputTarget (p2);
    icp.align (*output, guess);

    std::cerr<< "Final is " << std::endl << icp.getFinalTransformation() << std::endl;
    output->resize(p2->size()+output->size());
    for (int i=0;i<p2->size();i++)
    {
        output->push_back(p2->points[i]);
    }
    return output;
}


}


#endif

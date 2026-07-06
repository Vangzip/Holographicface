#ifndef IMAGE_OPERATE_HPP__
#define IMAGE_OPERATE_HPP__


#include <opencv2/opencv.hpp>
#include "pcl/point_cloud.h"
#include "pcl/point_types.h"
namespace PointcloudOperate
{

template <int min, int max>
std::tuple<uchar, uchar, uchar> getColor(double dis)
{
    int val = (dis - min) * 1536 / (max - min);
    // printf("%d\n", val);
    //  对视差图进赋颜色
    uchar cb = 0;
    uchar cg = 0;
    uchar cr = 0;
    if (val == 0)
    {
        cr = 0;
        cb = 0;
        cg = 0;
    }
    else if (val < 256)
    {
        cr = 0;
        cb = 255;
        cg = val;
    }
    else if (val <= 512)
    {
        cr = 0;
        cg = 255;
        cb = 512 - val;
    }
    else if (val < 768)
    {
        cg = 255;
        cb = 0;
        cr = val - 512;
    }
    else if (val < 1024)
    {
        cr = 255;
        cb = 0;
        cg = 1023 - val;
    }
    else if (val < 1280)
    {
        cr = 255;
        cb = val - 1024;
        cg = 0;
    }
    else if (val <= 1536)
    {
        cr = 1536 - val;
        cb = 255;
        cg = 0;
    }

    return {cr, cg, cb};
}

std::tuple<cv::Point2d, cv::Point3d> translate( cv::Point3d ptIn,  cv::Mat&mati ,cv::Mat& rt)
{
    cv::Mat pt4 = (cv::Mat_<double>(4, 1) <<ptIn.x,ptIn.y,ptIn.z, 1);
    //std::cout<< "pt4 is :" << pt4 << std::endl;
    cv::Mat ptTrans = rt * pt4;
    //std::cout<< "ptTrans is :" << ptTrans << std::endl;
    cv::Mat pt3 = ptTrans(cv::Rect(0, 0, 1, 3));
    //std::cout<< "pt3 is :" << pt3 << std::endl;
    double z = pt3.at<double>(2, 0);
    cv::Mat img = mati * pt3;
    //std::cout<< "img is :" << img << std::endl;
    cv::Point3d pt3D = cv::Point3d(pt3.at<double>(0,0), pt3.at<double>(1,0),pt3.at<double>(2,0));
    cv::Point2d ptImg = cv::Point2d(img.at<double>(0,0)/z, img.at<double>(1,0)/z);
    return std::make_tuple(ptImg, pt3D);
}

static bool remapPointcloudToRGBD( pcl::PointCloud<pcl::PointXYZRGB>::Ptr pc,
                                   int iWidth, int iHeight,
                                   cv::Mat i, cv::Mat rt,
                                   cv::Mat&rgb, cv::Mat& depth, bool bColoredByDis = false, float fScale= 1.0)
{
    if(rgb.empty() )
    {
        rgb = cv::Mat::zeros(iHeight, iWidth, CV_8UC3);
    }

    depth = cv::Mat::zeros(iHeight, iWidth, CV_32FC3);
    for(auto pt : *pc)
    {
        cv::Point3d ptCV(pt.x, pt.y, pt.z);
        auto rs = translate(ptCV,i,rt);
        cv::Point2d pt2d = std::get<0>(rs);
        cv::Point3d pt3d = std::get<1>(rs);

        if(pt2d.x <  0 ||pt2d.y <0 ||pt2d.x >= iWidth ||pt2d.y >=iHeight )
        {
            continue;
        }

        float dis = depth.ptr<float>(pt2d.y, pt2d.x)[2];
        if(    std::fabs(dis) < 0.00001     // 此时的深度为0，意味着没有赋值过
            || std::fabs(pt3d.z) <std::fabs(dis) // 新来的点的距离更近，则覆盖距离远的
        )
        {
            depth.ptr<float>(pt2d.y, pt2d.x)[0] = pt3d.x/fScale;
            depth.ptr<float>(pt2d.y, pt2d.x)[1] = pt3d.y/fScale;
            depth.ptr<float>(pt2d.y, pt2d.x)[2] = pt3d.z/fScale;

            if(bColoredByDis )
            {
                auto t = getColor<0,100>(pt3d.z/fScale);
                rgb.ptr<uchar>(pt2d.y, pt2d.x)[0] = std::get<2>(t);
                rgb.ptr<uchar>(pt2d.y, pt2d.x)[1] = std::get<1>(t);
                rgb.ptr<uchar>(pt2d.y, pt2d.x)[2] = std::get<0>(t);
            }
            else
            {
                rgb.ptr<uchar>(pt2d.y, pt2d.x)[0] = pt.b;
                rgb.ptr<uchar>(pt2d.y, pt2d.x)[1] = pt.g;
                rgb.ptr<uchar>(pt2d.y, pt2d.x)[2] = pt.r;
            }
        }
    }
    return true;
}

}


#endif

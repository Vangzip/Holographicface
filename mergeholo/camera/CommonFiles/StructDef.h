#ifndef STRUCTDEF_H
#define STRUCTDEF_H

#include "opencv2/opencv.hpp"
namespace JP {
// 单个相机数据结构体
using stCameraParam = struct _cameraParam
{
    int iDbId;
    std::string strUuid;

    double fx =0.0;     // 焦距
    double fy =0.0;     //
    double cx =0.0;     // 中心点
    double cy =0.0;     //
    double k0 =0.0;     // 畸变
    double k1 =0.0;     //
    double k2 =0.0;     //
    double p0 =0.0;     //
    double p1 =0.0;     //
    double s0 =0.0;     //

    _cameraParam& operator*=(double d)
    {
        fx *= d;
        fy *= d;
        cx *= d;
        cy *= d;
        k0 *= d;
        k1 *= d;
        k2 *= d;
        p0 *= d;
        p1 *= d;
        s0 *= d;
        return *this;
    }
};

// 双目相机参数
using stSteoroParam = struct _steoroParam
{
    // 分辨率
    int iWidth  =2048;
    int iHeight =1080;

    // 基线长度
    double dBaseline =0.0;

    // 右目相对左目的平移矩阵
    std::vector<float> vctTMatrix = {0,0,0};
    std::vector<float> vctRMatrix = {1, 0, 0,
                                      0, 1, 0,
                                      0, 0, 1};
    std::vector<float> vctRTMatrix = {1, 0, 0, 0,
                                       0, 1, 0, 0,
                                       0, 0, 1, 0,
                                       0, 0, 0, 1};

    cv::Rect roi;
    // 左目相机内参
    stCameraParam left;
    // 右目相机内参
    stCameraParam right;

    cv::Mat remap00;
    cv::Mat remap01;
    cv::Mat remap10;
    cv::Mat remap11;


    std::vector<double> vctQ= std::vector<double>(16);

    struct _steoroParam& operator *=( double d)
    {
        iWidth *= d;
        iHeight *= d;

        //for(int i=0; i< vctRMatrix.size(); i++ )
        //{
        //    vctRMatrix[i] *= d;
        //}

        for(size_t i=0; i< vctTMatrix.size(); i++ )
        {
            vctTMatrix[i] *= d;
        }

        left *= d;
        right *= d;

        return *this;
    }

    int idx;
    int iDisplycardId;

    int iDbId;
// 临时用一下	
    int ciGridSize = 1;

    double dScale = 1.0/2;

    double dOutputScale = 1.0/2;

    double dThres = 0.45;

    bool bShowDepth = false;

    bool bImgColor = true;

    double dRsScale = 1.0;
    int iGpu = 0;
};

using stDispalyCard = struct _st_display_card
{
    int idx;
    int function;
};


#pragma pack(1)
using stRGBD = struct _st_RGBD
{
    char r;
    char g;
    char b;
    float d;
};
#pragma pack()
}





#endif // STRUCTDEF_H

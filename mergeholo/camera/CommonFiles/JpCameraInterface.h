/**
 * @file JpCameraInterface.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-11-04
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef __JP_CAMERA_INTERFACE__
#define __JP_CAMERA_INTERFACE__
#include <cstdint>
#include <memory>
#include "opencv2/opencv.hpp"
namespace JP {


template <typename T >
struct camera_op_traits;


class IJpCameraInterface {
public :
    union Header
        {
            char arr[4];
            int v;
        };

        struct _struct_camera_frame
        {
            _struct_camera_frame()
            {
                timestamp = 0;
                //iLen = 0;
                //pBuffer = nullptr;
                vctData.resize(0);
            }
            int64_t timestamp =0;

            int getHeader3()
            {
                if( vctData.size()<3 )
                {
                    return 0;
                }
                Header o;
                o.arr[0] = 0;
                o.arr[1] = vctData[0];
                o.arr[2] = vctData[1];
                o.arr[3] = vctData[2];
                //printf("%x %x %x\n", vctData[0],vctData[1],vctData[2]);
                return o.v;
            }

            char type()
            {

                int v = getHeader3();
                if( v == 0 )
                {
                    if( vctData.size()<5 )
                    {
                        return 0;
                    }
                    else
                    {
                        return vctData[4];
                    }
                }
                else
                {
                    if( vctData.size()<4 )
                    {
                        return 0;
                    }
                    else
                    {
                        return vctData[3];
                    }
                }
            }

            bool isFrame()
            {

                char t = type();
                return (t == 0x26 || t == 0x02 );
            }

            bool isvsp()
            {

                char t = type();
                return (t == 0x40 || t == 0x42 || t == 0x44 );
            }

            bool isIFrame()
            {
                char t = type();

                return (t ==0x26 );
            }

    #ifdef JP_USE_OPENCV
            cv::Mat img;
            bool empty()
            {
                return img.empty();
            }
    #endif
            std::vector<unsigned char> vctData;



            void copyFrom( _struct_camera_frame& other )
            {
    #ifdef JP_USE_OPENCV
                img = other.img.clone();
    #endif
                timestamp = other.timestamp;
                //iLen = other.iLen;
                vctData = other.vctData;
            }

        } ;

        using TypeCameraFrame = _struct_camera_frame;

        using Ptr = std::shared_ptr<IJpCameraInterface>;

        using CameraStatus = enum
        {
            Unknown,
            Ready,
            Running,
            Pausing,
            Paused,
            Stopping,
            Stopped,
            Error
        };
public:
    /// 初始化相机
    // \param configstring 初始化字符串，暂时只传ip就够了， e.g. "10.10.0.31"
    // \param mode         触发模式 0 -自动触发 1-外触发 没有可以不使用
    virtual bool Init(const char* /*configstring*/, size_t /*szCameraIdx*/ = 0) =0;
    /// 卸载相机
    virtual void UnInit() =0;

    /// 重置设备
    virtual bool ResetDev() =0;

    /// 获得一帧图像
    // @fid 	输出，所获得的帧的序号
    // @timstamp 	输出，所获得的帧的时间戳
    // @len 	输出，所获得的帧的长度
    // 返回值	该帧的数据头指针，用户在外层释放
    virtual uint8_t* GetImage(uint64_t& /*fid*/, int64_t& /*timestamp*/, int& /*len*/) =0;


    virtual int GetImage(uint8_t* /*pData*/, size_t /*szInputLen*/,
                         uint64_t& /*fid*/, int64_t& /*timestamp*/) =0;

    virtual int GetImage(cv::Mat& /*img*/,
                         uint64_t& /*fid*/, int64_t& /*timestamp*/) =0;

    virtual bool GetImage(TypeCameraFrame& /*frame*/, int /*iTimeout*/ =0 ) =0;



public:
    /// 初始化相机的帧序号，
    virtual void ClearFID(){}

    /// 重置相机流
    virtual void ResetStream(){}

        virtual void setDecodeFlag(bool  ){}
        virtual CameraStatus GetStatus(){return Unknown;}
};
}

#endif //__JP_CAMERA_INTERFACE__

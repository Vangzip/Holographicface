#ifndef LightFieldCapture_H
#define LightFieldCapture_H

#include <iostream>
#include <memory>
#include <thread>
#include <tuple>
#include <functional>
#include <opencv2/core.hpp>
// #include "LinuxSerlPortImpl.h"

#include "JpICamera.h"
#include "JpIParse.h"

#include<CommonFiles/threadsafe_queue.hpp>
#include <CommonFiles/JPDeviceInterface.h>


class LightFieldCapture : public JP::IDeviceInterface
{
public:
/**
 * @struct HoloParam
 * @brief 光场相机参数结构体，包含曝光模式、曝光值、相机ID、帧率、阈值。
 */
    using HoloInData = struct _holo_in_data
    {
        int iHoloExposeMode; // 曝光模式
        int iHoloExposeVal; // 曝光值
        int iHoloId; // 相机ID
        double dHoloFrameRate; // 帧率
        int iHoloMissThreshold; // 阈值
        bool bIsReadTeamptureBySerial; // 是否通过串口读取温度
        std::string strSerialPort; // 串口端口
        int iSerialBaudRate; // 串口波特率
        int iSerialDataBits; // 串口数据位
        int iSerialStopBits; // 串口停止位
        int iSerialParity; // 串口校验位

        std::string strParseCfgPath;      // 解析配置文件路径
    };

    using HoloOutData = struct _holo_out_data
    {
        cv::Mat img2d;       // 2D图像数据
        cv::Mat img3d;      // 3D图像数据
        cv::Mat depthMap;   // 深度图像数据
        cv::Mat rawData;    // 原始数据
        jp_lightfield::timeval tv;  // 时间戳
        float tempreture[3];        //温度
    } ;


public:
    LightFieldCapture();
    ~LightFieldCapture();

public:
 /**
         * @brief 初始化设备
         * @param config 配置参数，指向设备特定的配置
         * @return 返回初始化是否成功
         */
    virtual bool initialize(const void *config) override;
    
             /**
         * @brief 释放设备资源
         */
         virtual void release() override;
 /**
         * @brief 打开设备
         * @return 总是返回 true，表示打开操作成功。
         */
         virtual bool openDevice() override{return true;}
     /**
         * @brief 关闭设备
         * 空实现，关闭 CAN 设备时无需特定的操作。
         */
         virtual void closeDevice() override{}

        // 读取数据
        virtual int readData(char* buffer, int length) override{return 0;}

        // 写入数据
        virtual bool writeData(const char* buffer, int length) override{return true;}

        void StartCapture();
    // 提供读取队列数据的接口（保留兼容性）
    bool GetHoloOutData(HoloOutData& outHoloData);

private:
    void run();

    // 光场相机读取到的原始图像保存
    void save(jp_lightfield::strLightFieldInput& data);
    // 光场相机解析后的图像保存
    void save(jp_lightfield::strLightFieldOutput& data);

private:
    jp_lightfield::JpICamera* ICam{ NULL };
    jp_lightfield::JpIParse* IParse{ NULL };
    jp_lightfield::strCameraConf Conf;
    // std::shared_ptr<LinuxSerlPortImpl> serlPortImpl;

private:
    // std::function<void(HoloOutData&&)> m_callback;
    std::unique_ptr<std::thread> readThread_;
    bool m_is_stop{ false };
    bool m_isInitSuccess{ true };
    int m_rawdatacount{ 0 };
    JP::threadsafe_queue<HoloOutData> m_queueHoloData;
};




#endif

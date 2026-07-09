#ifndef LightFieldCapture_H
#define LightFieldCapture_H

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <opencv2/core.hpp>

#include "JpICamera.h"
#include "JpIParse.h"

#include <CommonFiles/threadsafe_queue.hpp>
#include <CommonFiles/JPDeviceInterface.h>

class LightFieldCapture : public JP::IDeviceInterface
{
public:
    using HoloInData = struct _holo_in_data
    {
        int iHoloExposeMode;
        int iHoloExposeVal;
        int iHoloId;
        double dHoloFrameRate;
        int iHoloMissThreshold;
        bool bIsReadTeamptureBySerial;
        std::string strSerialPort;
        int iSerialBaudRate;
        int iSerialDataBits;
        int iSerialStopBits;
        int iSerialParity;

        std::string strParseCfgPath;
        int iGpuId{ 0 };
        std::string strCamSeri;
        std::string strCamType;
    };

    using HoloOutData = struct _holo_out_data
    {
        cv::Mat img2d;
        cv::Mat img3d;
        cv::Mat depthMap;
        cv::Mat rawData;
        jp_lightfield::timeval tv;
        float tempreture[3];
    };

public:
    LightFieldCapture();
    ~LightFieldCapture();

public:
    virtual bool initialize(const void *config) override;
    virtual void release() override;
    virtual bool openDevice() override { return true; }
    virtual void closeDevice() override {}
    virtual int readData(char* buffer, int length) override { return 0; }
    virtual bool writeData(const char* buffer, int length) override { return true; }

    void StartCapture();
    bool GetHoloOutData(HoloOutData& outHoloData);
    bool hasError() const;
    bool isRunning() const;
    std::string lastError() const;

private:
    void run();
    void setError(const std::string& message);
    void freeParseBuffers();

    void save(jp_lightfield::strLightFieldInput& data);
    void save(jp_lightfield::strLightFieldOutput& data);

private:
    jp_lightfield::JpICamera* ICam{ nullptr };
    jp_lightfield::JpIParse* IParse{ nullptr };
    jp_lightfield::strCameraConf Conf;

private:
    std::unique_ptr<std::thread> readThread_;
    std::atomic<bool> m_is_stop{ false };
    std::atomic<bool> m_isRunning{ false };
    std::atomic<bool> m_hasError{ false };
    mutable std::mutex m_errorMutex;
    std::string m_lastError;
    bool m_isInitSuccess{ true };
    int m_rawdatacount{ 0 };
    JP::threadsafe_queue<HoloOutData> m_queueHoloData;

    int m_fwidth{ 0 };
    int m_fheight{ 0 };
    int m_channel{ 1 };
    std::string m_camSeri;
    std::string m_camType;

    unsigned char* m_inputData{ nullptr };
    size_t m_inputDataSize{ 0 };

    unsigned char* m_resultImg2d{ nullptr };
    unsigned char* m_resultDepthMap{ nullptr };
    float* m_resultImg3d{ nullptr };
    float* m_resultImgTest{ nullptr };
};

#endif

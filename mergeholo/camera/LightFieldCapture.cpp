#include "LightFieldCapture.h"

#include <QDebug>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <exception>
#include <sstream>

#include <opencv2/imgproc.hpp>

struct strTempDataCh2
{
    double tem1;
    double tem2;
    strTempDataCh2() : tem1(-9999.0), tem2(-9999.0) {};
};

LightFieldCapture::LightFieldCapture()
{
}

LightFieldCapture::~LightFieldCapture()
{
    qDebug() << "LightFieldCapture::~LightFieldCapture() start";
    release();
    qDebug() << "LightFieldCapture::~LightFieldCapture() end";
}

void LightFieldCapture::freeParseBuffers()
{
    delete[] m_inputData;
    m_inputData = nullptr;
    m_inputDataSize = 0;

    delete[] m_resultImg2d;
    m_resultImg2d = nullptr;
    delete[] m_resultDepthMap;
    m_resultDepthMap = nullptr;
    delete[] m_resultImg3d;
    m_resultImg3d = nullptr;
    delete[] m_resultImgTest;
    m_resultImgTest = nullptr;
}

bool LightFieldCapture::initialize(const void *config)
{
    m_is_stop.store(false, std::memory_order_release);
    m_isRunning.store(false, std::memory_order_release);
    m_hasError.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastError.clear();
    }
    m_isInitSuccess = true;

    auto *p = reinterpret_cast<const HoloInData *>(config);
    if (!p) {
        return false;
    }

    qDebug() << "LightFieldCapture::Init() start";

    m_camSeri = p->strCamSeri.empty() ? "CXP" : p->strCamSeri;
    m_camType = p->strCamType;
    ICam = jp_lightfield::JpICamera::GetICamera(m_camSeri);

    Conf.exposeMode = p->iHoloExposeMode;
    Conf.exposeVal = p->iHoloExposeVal;
    Conf.id = p->iHoloId;
    Conf.frameRate = p->dHoloFrameRate;
    Conf.missThresold = p->iHoloMissThreshold;

    int ret = ICam->Init(Conf, m_camType);
    if (ret != 0) {
        qDebug() << "LightFieldCapture::Init() JpICamera Init failed";
        m_isInitSuccess = false;
        return m_isInitSuccess;
    }
    qDebug() << "LightFieldCapture::Init() JpICamera Init success";

    m_channel = Conf.bColor ? 3 : 1;

    IParse = jp_lightfield::JpIParse::GetIParse();

    std::string parseCfg = p->strParseCfgPath;
    qDebug() << "config camera path:" << parseCfg.c_str();

    ret = IParse->Init(parseCfg, p->iGpuId, m_fheight, m_fwidth);
    if (ret != 0) {
        qDebug() << "LightFieldCapture::Init() JpIParse Init failed";
        m_isInitSuccess = false;
        return m_isInitSuccess;
    }

    if (m_fheight <= 0 || m_fwidth <= 0) {
        setError("Camera parser returned invalid output size.");
        m_isInitSuccess = false;
        return m_isInitSuccess;
    }

    freeParseBuffers();
    const size_t pixelCount = static_cast<size_t>(m_fheight) * static_cast<size_t>(m_fwidth);
    m_resultImg2d = new unsigned char[pixelCount * static_cast<size_t>(m_channel)];
    m_resultDepthMap = new unsigned char[pixelCount * 3U];
    m_resultImg3d = new float[pixelCount * 3U];
    m_resultImgTest = new float[pixelCount];

    StartCapture();
    qDebug() << "LightFieldCapture::Init() finish";

    return m_isInitSuccess;
}

void LightFieldCapture::StartCapture()
{
    if (m_isInitSuccess) {
        readThread_ = std::make_unique<std::thread>(&LightFieldCapture::run, this);
    }
}

void LightFieldCapture::run()
{
    qDebug() << "LightFieldCapture::run()";
    m_isRunning.store(true, std::memory_order_release);

    jp_lightfield::strCameraData lightRowData;
    jp_lightfield::strLightFieldInput m_parseInput;
    jp_lightfield::strLightFieldOutput m_parseOut;

    m_parseOut.img2d = m_resultImg2d;
    m_parseOut.depthMap = m_resultDepthMap;
    m_parseOut.img3d = m_resultImg3d;
    m_parseOut.imgTest = m_resultImgTest;
    m_parseInput.channel = m_channel;

    int consecutiveCaptureFailures = 0;
    int consecutiveParseFailures = 0;
    const int maxConsecutiveCaptureFailures = 100;
    const int maxConsecutiveParseFailures = 30;

#if DEBUG_MODULE_LIGHT_LOG
    int frameCount = 0;
    auto startTime = std::chrono::system_clock::now();
#endif

    try {
        while (!m_is_stop.load(std::memory_order_acquire)) {
#if DEBUG_MODULE_LIGHT_LOG
            auto st = std::chrono::system_clock::now();
#endif
            int res = ICam->Capture(lightRowData);
            if (res != 0) {
                ++consecutiveCaptureFailures;
                if (consecutiveCaptureFailures >= maxConsecutiveCaptureFailures) {
                    std::ostringstream message;
                    message << "Camera capture failed " << consecutiveCaptureFailures
                        << " times consecutively, last code=" << res;
                    setError(message.str());
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                continue;
            }
            consecutiveCaptureFailures = 0;

#if DEBUG_MODULE_LIGHT_LOG
            auto st1 = std::chrono::system_clock::now();
#endif
            float temch1 = 0.0f;
            float temch2 = 0.0f;

            m_parseInput.dataLen = lightRowData.dataLen;
            const size_t bytesPerPixel = static_cast<size_t>(std::max(1, lightRowData.dataLen));
            const size_t sourceBytes = bytesPerPixel
                * static_cast<size_t>(Conf.height)
                * static_cast<size_t>(Conf.width);
            const size_t requiredInputSize = sourceBytes * static_cast<size_t>(m_channel);

            if (requiredInputSize > m_inputDataSize) {
                delete[] m_inputData;
                m_inputData = new unsigned char[requiredInputSize];
                m_inputDataSize = requiredInputSize;
            }

            const jp_lightfield::timeval captureTv = lightRowData.tv;
            const float captureTemperature = lightRowData.tempreture;

            if (!lightRowData.data || sourceBytes == 0 || !m_inputData) {
                ICam->Free(lightRowData);
                setError("Camera capture returned empty frame data.");
                break;
            }

            std::memcpy(m_inputData, lightRowData.data, sourceBytes);
            ICam->Free(lightRowData);

            m_parseInput.data = m_inputData;

            if (m_parseInput.channel == 3) {
                cv::ColorConversionCodes bayerFlag = cv::COLOR_BayerBG2BGR;
                if (m_camSeri == "CXP" && m_camType == "GM21M12X4") {
                    bayerFlag = cv::COLOR_BayerGR2BGR;
                }
                if (m_camSeri == "571" && m_camType == "MindVision") {
                    bayerFlag = cv::COLOR_BayerRG2BGR;
                }

                cv::Mat bayer(Conf.height, Conf.width, CV_8UC1, m_parseInput.data);
                cv::Mat color;
                cv::cvtColor(bayer, color, bayerFlag);
                std::memcpy(m_parseInput.data, color.data,
                    static_cast<size_t>(Conf.height) * static_cast<size_t>(Conf.width) * 3U);
            }

            m_parseInput.tempreture[0] = captureTemperature;
            m_parseInput.tempreture[1] = temch1;
            m_parseInput.tempreture[2] = temch2;

            res = IParse->Parse(m_parseInput, m_parseOut);
            if (res != 0) {
                ++consecutiveParseFailures;
                if (consecutiveParseFailures >= maxConsecutiveParseFailures) {
                    std::ostringstream message;
                    message << "Camera frame parse failed " << consecutiveParseFailures
                        << " times consecutively, last code=" << res;
                    setError(message.str());
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            consecutiveParseFailures = 0;

#if DEBUG_MODULE_LIGHT_LOG
            auto st3 = std::chrono::system_clock::now();
#endif

            cv::Mat img2d(m_fheight, m_fwidth,
                m_channel == 1 ? CV_8UC1 : CV_8UC3, m_parseOut.img2d);
            cv::Mat depthMap(m_fheight, m_fwidth, CV_8UC3, m_parseOut.depthMap);
            cv::Mat img3d(m_fheight, m_fwidth, CV_32FC3, m_parseOut.img3d);

            cv::Mat rawData;
            if (m_parseInput.dataLen == 1) {
                rawData = cv::Mat(Conf.height, Conf.width,
                    m_channel == 1 ? CV_8UC1 : CV_8UC3, m_parseInput.data);
            }
            else {
                rawData = cv::Mat(Conf.height, Conf.width, CV_16UC1, m_parseInput.data);
            }

            HoloOutData holoData;
            holoData.img2d = img2d.clone();
            holoData.img3d = img3d.clone();
            holoData.depthMap = depthMap.clone();
            holoData.rawData = rawData.clone();
            holoData.tv = captureTv;
            holoData.tempreture[0] = m_parseInput.tempreture[0];
            holoData.tempreture[1] = m_parseInput.tempreture[1];
            holoData.tempreture[2] = m_parseInput.tempreture[2];

            m_queueHoloData.push(holoData);

#if DEBUG_MODULE_LIGHT_LOG
            frameCount++;
            auto currentTime = std::chrono::system_clock::now();
            auto dur1 = std::chrono::duration_cast<std::chrono::milliseconds>(st1 - st).count();
            auto dur3 = std::chrono::duration_cast<std::chrono::milliseconds>(st3 - st1).count();
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - st).count();
            auto durcap = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch()).count() -
                ((uint64_t)captureTv.tv_sec * 1000 + (captureTv.tv_usec / 1000));

            auto secdur = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
            if (secdur >= 3000) {
                printf("LightFieldCapture frame count: %d\n", frameCount / 3);
                frameCount = 0;
                startTime = currentTime;
                printf("LightFieldCapture: capture: %llu, Parse: %llu, run: %llu durcap: %llu \n", dur1, dur3, dur, durcap);
            }
#endif
        }
    }
    catch (const std::exception& ex) {
        setError(std::string("Camera capture thread failed: ") + ex.what());
    }
    catch (...) {
        setError("Camera capture thread failed with an unknown exception.");
    }

    m_isRunning.store(false, std::memory_order_release);
}

void LightFieldCapture::save(jp_lightfield::strLightFieldInput &data)
{
    (void)data;
}

void LightFieldCapture::save(jp_lightfield::strLightFieldOutput &data)
{
    (void)data;
}

bool LightFieldCapture::GetHoloOutData(HoloOutData& outHoloData)
{
    return m_queueHoloData.pop(outHoloData);
}

bool LightFieldCapture::hasError() const
{
    return m_hasError.load(std::memory_order_acquire);
}

bool LightFieldCapture::isRunning() const
{
    return m_isRunning.load(std::memory_order_acquire);
}

std::string LightFieldCapture::lastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}

void LightFieldCapture::setError(const std::string& message)
{
    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastError = message;
    }
    m_hasError.store(true, std::memory_order_release);
    m_is_stop.store(true, std::memory_order_release);
    qDebug() << message.c_str();
}

void LightFieldCapture::release()
{
    qDebug() << "LightFieldCapture::release()";
    m_is_stop.store(true, std::memory_order_release);
    if (readThread_ && readThread_->joinable()) {
        readThread_->join();
        readThread_.reset(nullptr);
    }

    freeParseBuffers();

    if (IParse) {
        jp_lightfield::JpIParse::ReleaseIParse(IParse);
        IParse = nullptr;
    }

    if (ICam) {
        jp_lightfield::JpICamera::ReleaseICamera(ICam);
        ICam = nullptr;
    }
    m_isRunning.store(false, std::memory_order_release);
}

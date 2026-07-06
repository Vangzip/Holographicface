#include "LightFieldCapture.h"

// #include "SendMainDefine.h"
#include<QDebug>

#include <chrono>
#include <exception>
#include <sstream>


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
    qDebug() << "LightFieldCapture::~LightFieldCapture() start" ;
    release();
    qDebug() << "LightFieldCapture::~LightFieldCapture() end" ;
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

    auto *p = reinterpret_cast<const HoloInData *>(config); // 将配置参数转换为 HoloInData 结构体   
        if (!p)
        {
            return false;
        }
    qDebug() << ("LightFieldCapture::Init() start");
    ICam = jp_lightfield::JpICamera::GetICamera();

    // 从配置文件中读取配置参数（当前使用硬编码示例）
    // auto &cfg = CConfig::Get();
    // Conf.exposeMode = cfg.m_iHoloExposeMode1;
    // Conf.exposeVal[Conf.exposeMode] = cfg.m_iHoloExposeVal1;
    // Conf.id = cfg.m_iHolo1Id;
    // Conf.frameRate = cfg.m_iHoloFrameRate;
    // Conf.missThresold = cfg.m_iHoloThresholdMissFrame;

    // Conf.exposeMode = 1;
    // Conf.exposeVal[Conf.exposeMode] = 6000;
    // Conf.id = 0;
    // Conf.frameRate = 25;
    // Conf.missThresold = 1000;

    Conf.exposeMode = p->iHoloExposeMode;
    Conf.exposeVal[Conf.exposeMode] = p->iHoloExposeVal;
    Conf.id = p->iHoloId;
    Conf.frameRate = p->dHoloFrameRate;
    Conf.missThresold = p->iHoloMissThreshold;

    int ret = ICam->Init(Conf);
    if (ret != 0)
    {
        qDebug() << ("LightFieldCapture::Init() JpICamera Init failed");
        m_isInitSuccess = false;
        return m_isInitSuccess;
    }
    qDebug() << ("LightFieldCapture::Init() JpICamera Init success");

    if(1)
        IParse = jp_lightfield::JpIParse::GetIParse(jp_lightfield::parseQuick);
    else
        IParse = jp_lightfield::JpIParse::GetIParse(jp_lightfield::parseSlow);


    std::string parseCfg = p->strParseCfgPath; // 统一用正斜杠
    // std::string parseCfg = "D:/ljc-work-dir/holoCamera/00-bin/config/182C/"; // 统一用正斜杠
    qDebug() << "config camera path:" << parseCfg.c_str(); // 输出：config camera path: "D:/.../182C/"

    ret = IParse->Init(parseCfg, 0);

    if (0 != ret)
    {
        qDebug() << ("LightFieldCapture::Init() JpIParse Init failed");
        m_isInitSuccess = false;
        return m_isInitSuccess;
    }

    // 初始化温度串口
    if(p->bIsReadTeamptureBySerial)
    {
        // serlPortImpl = std::make_shared<LinuxSerlPortImpl>(p->strSerialPort.c_str());

    }

    // m_callback = callback;

    StartCapture();
    qDebug() << ("LightFieldCapture::Init() finish");

    return m_isInitSuccess;
}

void LightFieldCapture:: StartCapture() 
{
    if(m_isInitSuccess)
        readThread_ = std::make_unique<std::thread>(&LightFieldCapture::run, this);

    // if(serlPortImpl)
    //     serlPortImpl->initialize();
}


void LightFieldCapture::run()
{
    qDebug() << ("LightFieldCapture::run()");
    m_isRunning.store(true, std::memory_order_release);

    jp_lightfield::strCameraData lightRowData;      //光场原始数据
    jp_lightfield::strLightFieldInput m_parseInput; // 解析接口的入参数据结构
    jp_lightfield::strLightFieldOutput m_parseOut; // 解析接口的出参数据结构
    int consecutiveCaptureFailures = 0;
    int consecutiveParseFailures = 0;
    const int maxConsecutiveCaptureFailures = 100;
    const int maxConsecutiveParseFailures = 30;


#if DEBUG_MODULE_LIGHT_LOG
    int frameCount = 0;     // 帧率计数器
    auto startTime = std::chrono::system_clock::now();
#endif
    //循环读取
    try
    {
    while(!m_is_stop.load(std::memory_order_acquire))
    {
#if DEBUG_MODULE_LIGHT_LOG
        auto st = std::chrono::system_clock::now();
#endif
        int res = ICam->Capture(lightRowData);
        if (res != 0){      // 结果=0表示读取数据成功
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
        float temch1 = 0.0;
        float temch2 = 0.0;
//        while(!serlPortImpl->get_data(temch1, temch2) && !m_is_stop)
//        {
//            continue;
//        }
#if DEBUG_MODULE_LIGHT_LOG
        auto st2 = std::chrono::system_clock::now();
#endif
        // 光场解析
        m_parseInput.data = lightRowData.data;
        m_parseInput.bmono = lightRowData.bmono;
        m_parseInput.tempreture[0] = lightRowData.tempreture;
        m_parseInput.tempreture[1] = temch1;
        m_parseInput.tempreture[2] = temch2;

         res = IParse->Parse(m_parseInput, m_parseOut);
        if (res != 0){      // 结果=0表示读取数据成功
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

        HoloOutData holoData;
        holoData.img2d = m_parseOut.img2d.clone();
        holoData.img3d = m_parseOut.img3d.clone();
        holoData.depthMap = m_parseOut.depthMap.clone();
        holoData.rawData = lightRowData.data.clone();
        holoData.tv = lightRowData.tv;
        holoData.tempreture[0] = m_parseInput.tempreture[0];
        holoData.tempreture[1] = m_parseInput.tempreture[1];
        holoData.tempreture[2] = m_parseInput.tempreture[2];

        // if(m_callback)
        // {
        //     m_callback(std::move(holoData));
        // }
        m_queueHoloData.push(holoData);
        
       
#if DEBUG_MODULE_LIGHT_LOG
        frameCount++;
        auto currentTime = std::chrono::system_clock::now();
        auto dur1 = std::chrono::duration_cast<std::chrono::milliseconds>(st1 - st).count();            // capture
        auto dur2 = std::chrono::duration_cast<std::chrono::milliseconds>(st2 - st1).count();           // temperature
        auto dur3 = std::chrono::duration_cast<std::chrono::milliseconds>(st3 - st2).count();           // Parse
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - st).count();
        auto durcap = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch()).count() -
                      ((uint64_t)lightRowData.tv.tv_sec * 1000 + (lightRowData.tv.tv_usec / 1000));

        auto secdur = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
        if(secdur >= 3000)
        {
            printf("LightFieldCapture frame count: %d\n", frameCount / 3); // frame 平均= 3
            // 重置计数器和时间
            frameCount = 0;
            startTime = currentTime;
            printf("LightFieldCapture: capture: %llu, temperature: %llu, Parse: %llu, run: %llu durcap: %llu \n", dur1, dur2, dur3, dur, durcap);      // dur 平均= 230ms
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
    // if(CConfig::Get().m_cameraDataIsSave && m_rawdatacount < CConfig::Get().m_cameraDataCount)
    // {
    //     std::string filepath = ToolFunc::getExecutableDirectory() + "/" + CConfig::Get().m_holoRawDataSavePath;
    //     // 构建文件名
    //     std::string filename = filepath + "/" + std::to_string(m_rawdatacount) + ".tif";
    //     if(!cv::imwrite(filename, data.data))
    //     {
    //         LOGE("LightFieldCapture::save [failed] filename: %s", filename.c_str());
    //         CConfig::Get().m_cameraDataIsSave = false;
    //     }
    //     qDebug()<<("LightFieldCapture::save [success] filename: " << filename)
    //     m_rawdatacount++;
    // }
}

void LightFieldCapture::save(jp_lightfield::strLightFieldOutput &data)
{
    // if(CConfig::Get().m_cameraDataIsSave && m_rawdatacount < CConfig::Get().m_cameraDataCount)
    // {
    //     std::string filepath = ToolFunc::getExecutableDirectory() + "/" + CConfig::Get().m_holoRawDataSavePath;
    //     // 构建文件名
    //     std::string filename = filepath + "/" + std::to_string(m_rawdatacount) + ".jpg";
    //     cv::Mat resimg;
    //     cv::hconcat(data.img2d, data.depthMap, resimg);
    //     cv::imwrite(filename, resimg);
    //     m_rawdatacount++;
    // }
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

 /*
int LightFieldCapture::readData(char* buffer, int length)
{
  qDebug() << ("LightFieldCapture::readData() start");
    if (!buffer || length <= 0) {
        return -1;
    }
    
    // 计算需要的数据大小
    size_t dataSize = sizeof(HoloOutData);
    if (static_cast<size_t>(length) < dataSize) {
        return -2; // 缓冲区太小
    }
    
    // 直接从队列中获取数据
    HoloOutData outHoloData;
    if (m_queueHoloData.pop(outHoloData)) {
        // 成功获取数据，复制到缓冲区
        memcpy(buffer, &outHoloData, dataSize);
        return static_cast<int>(dataSize);
    } else {
        // 队列中没有数据
        return 0;
    } 
}*/

void LightFieldCapture::release()
{
    qDebug() << ("LightFieldCapture::release()");
    m_is_stop.store(true, std::memory_order_release);
    if(readThread_ && readThread_->joinable())
    {
        readThread_->join();
        readThread_.reset(nullptr);
    }

    if(IParse) {
        jp_lightfield::JpIParse::ReleaseIParse(IParse);
        IParse = nullptr;
    }

    if(ICam) {
        jp_lightfield::JpICamera::ReleaseICamera(ICam);     
        ICam = nullptr;
    }
    m_isRunning.store(false, std::memory_order_release);
}




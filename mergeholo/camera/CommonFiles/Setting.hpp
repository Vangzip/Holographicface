  ////////////////////////////////////////////////////////////////////////////////
// 文件名: MachineInfoMgr.hpp
// 类  名: CInfoMgrBase 
// 描  述: 配置信息管理文件
// 创建者:罗志彬
// 日  期:2019-10-31
////////////////////////////////////////////////////////////////////////////////
#ifndef MACHINE_INFO_MGR_HPP
#define MACHINE_INFO_MGR_HPP

#include <QSettings>
#include "Singleton.hpp"


#include <QDebug>
class CInfoMgrBase
{
public:
    CInfoMgrBase( QString strFile )
        :m_setting(strFile, QSettings::IniFormat)
    {
    }
    virtual ~CInfoMgrBase()
    {

    }

    // 从配置文件中读取信息
    virtual void Load()
    {
        // 服务器信息，
        m_strServerIp = m_setting.value("server/ip").toString();
        m_iPort = m_setting.value("server/port").toInt();
    }

    // 设置服务器信息
    bool SetServerInfo( QString strIp, int iPort )
    {
        m_strServerIp = strIp;
        m_iPort = iPort;

        m_setting.setValue("server/ip",strIp);
        m_setting.setValue("server/port",iPort);
        return true;
    }

    // 是否经过初始化
    bool IsInited()
    {
        return m_bIsInit;
    }
protected:
    QSettings m_setting;
    bool m_bIsInit;
public:
    QString m_strServerIp;
    int m_iPort;
    QString m_strSoftwareVersion = "V 1.0.0.0001";
};


class CSettingInfo : public CInfoMgrBase
{
public:
    using SetSingleton = Singleton<CSettingInfo>;
    // 获得静态实例
    static CSettingInfo& Get()
    {
        static CSettingInfo static_CSettingInfo("Config.ini");
        if( !static_CSettingInfo.IsInited() )
        {// 如果没有初始化，就读一下配置
            static_CSettingInfo.Load();
        }

        return static_CSettingInfo;
    }
public:
    using CInfoMgrBase::CInfoMgrBase;

    virtual void Load()
    {
        CInfoMgrBase::Load();
        m_strSlidePortName = m_setting.value("Common/SlideRailPortName").toString();
        m_bIsEntrance = m_setting.value("Common/isEntrance").toBool();
        m_strHolo1ParsePat = m_setting.value("Common/holoCamParseCfg1").toString();
//        m_strHolo2ParsePat = m_setting.value("Common/holoCamParseCfg2").toString();
        m_slideMaxPos = m_setting.value("Common/slideMaxPos").toInt();
        m_slideStep = m_setting.value("Common/slideStep").toInt();
        m_strRtspUrlEntrance = m_setting.value("Common/rtspUrlEntrance").toString();
        m_strRtspUrlDepartDetect = m_setting.value("Common/rtspUrlDepartDetect").toString();
        m_strRtspUrlDepart2dCollect = m_setting.value("Common/rtspUrlDepart2dCollect").toString();
        m_iHoloNumsEntrance = m_setting.value("Common/holoCapturingNumsEntrance").toInt();
        m_iCaptureSleepTm = m_setting.value("Common/captureSleepTm").toInt();
        m_slideSpeed = m_setting.value("Common/slideSpeed").toInt();
        m_slideAcc = m_setting.value("Common/slideAccelerate").toInt();
        m_save = m_setting.value("Temp/save").toBool();
        m_iDetectTest = m_setting.value("Temp/DetectTest").toInt();
        m_iHoloExposeMode1 = m_setting.value("Common/holoExposeMode1").toInt();
        m_iHoloExposeVal1 = m_setting.value("Common/holoExposeVal1").toInt();
        m_iHoloExposeMode2 = m_setting.value("Common/holoExposeMode2").toInt();
        m_iHoloExposeVal2 = m_setting.value("Common/holoExposeVal2").toInt();
        m_bManualCollect = m_setting.value("Temp/manualCollect").toBool();
        m_iTruckMoveDistance = m_setting.value("Common/truckMoveDistance").toInt();
        m_iSlideRailDiff = m_setting.value("Common/slideDiff").toInt();
        m_strCoalType = m_setting.value("Temp/coalType").toString();
        m_iLogRate = m_setting.value("Commmon/logRate").toInt();
        m_iTruckSilentComparedTm = m_setting.value("Common/truckSilentComparedTm").toInt();
        m_slideZeroDiff = m_setting.value("Common/slideZeroDiff").toInt();

        m_strCoalDensityRange=m_setting.value("Temp/coal").toString();

        m_strCoal3DensityRange=m_setting.value("Temp/coal3").toString();
        m_strCoal4DensityRange=m_setting.value("Temp/coal4").toString();
        m_strCoal5DensityRange=m_setting.value("Temp/coal5").toString();
        m_strBigBlockDensityRange=m_setting.value("Temp/bigBlock").toString();
        m_strCoalSlimeDensityRange=m_setting.value("Temp/coalSlime").toString();
        m_strStoneDensityRange=m_setting.value("Temp/stone").toString();
        m_strPeaCoalDensityRange=m_setting.value("Temp/peaCoal").toString();


        selfTest = m_setting.value("Temp/selfTest").toBool();
        m_bSkip2dCamera = m_setting.value("Temp/skip2dCamera").toBool();
        discardFrames = m_setting.value("Temp/discardFrames").toInt();
        waitSteadyTm = m_setting.value("Temp/waitSteadyTm").toInt();
        m_bTestParse = m_setting.value("Temp/testParse").toBool();
        m_bTestStitch = m_setting.value("Temp/testStitch").toBool();
        m_bDrawBox = m_setting.value("Temp/drawBox").toBool();
        m_iTruckSilentComparedTimes = m_setting.value("Common/truckSilentComparedTimes").toInt();
        m_iHoloCamNum = m_setting.value("Temp/holoCamNum").toInt();
        parsingStitch = m_setting.value("Temp/parsingStitch").toInt();
        parseWaitLimit = m_setting.value("Temp/parseWaitLimit").toInt();
        createStitchPly = m_setting.value("Temp/createStitchPly").toBool();

        m_udpIp = m_setting.value("udpDataDeliver/Ip").toString().toStdString();
        m_udpPort = m_setting.value("udpDataDeliver/Port").toInt();
        m_simu = m_setting.value("Temp/simuTruckTrigger").toBool();
        udpRecieve = m_setting.value("Temp/udpReceive").toBool();
        m_iBrightness = m_setting.value("Common/LightBrightness").toInt();
        m_iHoloFrameRate = m_setting.value("Common/HoloFrameRate").toInt();
        m_iHoloThresholdMissFrame = m_setting.value("Common/HoloMissThreshold").toInt();
        m_bTestTofStitch = m_setting.value("Temp/TestTofStitch").toBool();
        m_bWeightManullyInput = m_setting.value("Temp/weightManuallyInput").toBool();
        m_iSensorNum = m_setting.value("Temp/SensorNum").toInt();
//        m_iCleanLevel       =   m_setting.value("Compute/CleanLevel").toInt();
//        m_dOutputScale      =   m_setting.value("Compute/OutputScale").toDouble();
//        m_dComputeScale     =   m_setting.value("Compute/ComputeScale").toDouble();
//        m_dMaxDepth         =   m_setting.value("Compute/MaxDepth").toDouble();
//        m_iGridSize         =   m_setting.value("Compute/GridSize").toInt();
//        m_bCsv              =   m_setting.value("Output/csv").toBool();
//        m_bTiff             =   m_setting.value("Output/tiff").toBool();
//        m_bPointcloud       =   m_setting.value("Output/pointcloud").toBool();
//        m_strLeft           =   m_setting.value("Path/left").toString();
//        m_strRight          =   m_setting.value("Path/right").toString();


        m_iHoloExposeMode1Sensor = m_setting.value("Temp/holoExposeMode1Sensor").toInt();
        m_iHoloExposeVal1Sensor = m_setting.value("Temp/holoExposeVal1Sensor").toInt();
        m_iHoloExposeMode2Sensor = m_setting.value("Temp/holoExposeMode2Sensor").toInt();
        m_iHoloExposeVal2Sensor = m_setting.value("Temp/holoExposeVal2Sensor").toInt();
    }

    virtual void Save()
    {
        qDebug() << "Save   ...";
        m_setting.setValue("Common/SlideRailPortName", m_strSlidePortName);
        m_setting.setValue("Common/isEntrance", m_strSlidePortName);
        m_setting.setValue("Temp/coalType1", m_strCoalType);
        CInfoMgrBase::SetServerInfo(m_strServerIp,m_iPort);

//        m_setting.setValue("Compute/CleanLevel",m_iCleanLevel );
//        m_setting.setValue("Compute/OutputScale",m_dOutputScale );
//        m_setting.setValue("Compute/ComputeScale",m_dComputeScale );
//        m_setting.setValue("Compute/GridSize",m_iGridSize );
//        m_setting.setValue("Compute/MaxDepth",m_dMaxDepth );
//        m_setting.setValue("Output/csv",m_bCsv );
//        m_setting.setValue("Output/tiff",m_bTiff );
//        m_setting.setValue("Output/pointcloud",m_bPointcloud );
//        m_setting.setValue("Path/left",m_strLeft );
//        m_setting.setValue("Path/right",m_strRight );
    }
public:
    QString m_strSlidePortName = "COM7";//滑轨端口名
    bool m_bIsEntrance=0;//是入场流程还是出场流程
    QString m_strHolo1ParsePat = "038C";//光场相机1的解析配置文件夹
//    QString m_strHolo2ParsePat = "053C";
    int m_slideMaxPos = 10000;//滑轨的最末端位置，可滑动的最大距离
    int m_slideStep = 25000; //滑轨步进 的幅度距离
    int m_slideSpeed = 500; //滑轨移动速度
    int m_slideAcc = 5000; // 滑轨加速度
    int m_rtspImgWidth = 1920;//rtsp图像 宽度
    int m_rtspImgHeight = 1080;//rtsp图像 高度
    QString m_strRtspUrlEntrance = "";
    QString m_strRtspUrlDepartDetect = "";
    QString m_strRtspUrlDepart2dCollect = "testData/";
    int m_iHoloNumsEntrance = 5;
    int m_iCaptureSleepTm = 1000;
    int m_iDetectTest =0;
    //光场相机的曝光参数：曝光模式和手动曝光值
    int m_iHoloExposeMode1 = 1;
    int m_iHoloExposeVal1 = 20000;
    int m_iHoloExposeMode2 = 1;
    int m_iHoloExposeVal2 = 20000;
    int m_iTruckMoveDistance=10;
    int m_iSlideRailDiff=50;
    int m_iLogRate=500;
    int m_iTruckSilentComparedTm=100;
    int m_slideZeroDiff=150;


    QString m_strCoalType="无烟";
    bool m_save = 0;//临时保存rtsp图像文件用
    bool m_bManualCollect = 0;//临时跳过出问题的前视相机拉流环节，手动触发出场时 滑轨移动和光场拍摄的后续流程



    bool m_bSkip2dCamera=0;
    bool selfTest=0;
    int discardFrames=3;
    int waitSteadyTm=500;
    bool m_bTestParse=0;
    bool m_bDrawBox=0;
    int m_iTruckSilentComparedTimes=5;
    int m_iHoloCamNum=2;
    bool m_bTestStitch=0;
//    bool m_bInit

    bool parsingStitch=0;
    int parseWaitLimit=5 * 1000 * 1000;
    bool createStitchPly=0;

    QString m_strCoalDensityRange="1.0-1.1";

    QString m_strCoal3DensityRange="1.0-1.1";
    QString m_strCoal4DensityRange="1.0-1.1";
    QString m_strCoal5DensityRange="1.0-1.1";
    QString m_strBigBlockDensityRange="1.0-1.1";
    QString m_strCoalSlimeDensityRange="1.0-1.1";
    QString m_strStoneDensityRange="1.0-1.1";
    QString m_strPeaCoalDensityRange="1.0-1.1";

    std::string m_udpIp = "";
    int m_udpPort = 8088;

    uint8_t m_iBrightness = 50;
    bool m_simu = 0; //模拟来车

    bool udpRecieve = false;
    int m_iHoloFrameRate = 10; //光场相机每秒的帧率
    int m_iHoloThresholdMissFrame=0;//光场相机丢包阈值
    bool m_bWeightManullyInput = false;//混装测试时，是否手动输入重量
    bool m_bTestTofStitch=0;

    size_t m_iSensorNum=16;//传感器数量
//    int     m_iCleanLevel       = 3;
//    double  m_dOutputScale      = 0.25;
//    double  m_dComputeScale     = 0.40;
//    double  m_dMaxDepth         = 100;
//    int     m_iGridSize         = 1;
//    bool    m_bCsv              = true;
//    bool    m_bTiff             =false;
//    bool    m_bPointcloud       = true;
//    QString m_strLeft           = "";
//    QString m_strRight          = "";
//    bool m_bLog = false;

    int m_iHoloExposeMode1Sensor;
    int m_iHoloExposeVal1Sensor;
    int m_iHoloExposeMode2Sensor;
    int m_iHoloExposeVal2Sensor;
};

#endif

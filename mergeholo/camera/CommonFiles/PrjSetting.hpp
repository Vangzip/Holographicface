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


    virtual bool Save()
    {
        return SetServerInfo(m_strServerIp,m_iPort);
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



    QString value(QString strName, QString defaultVal="" )
    {
        return m_setting.value(strName, defaultVal).toString();
    }


    void setValue( QString strName ,QString strValue )
    {
        m_setting.setValue(strName, strValue);
    }

    template< typename T,
              std::enable_if_t< std::is_arithmetic_v<std::remove_cv_t<T> > >* = nullptr>
    void setValue( QString strName, const T& value )
    {
        setValue(strName, QString::number(value));
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
       m_iShowTopView =  m_setting.value("Common/ShowTopView").toInt();
       m_iShowCircleView = m_setting.value("Common/ShowCircleView").toInt();
       m_iWaringLevel = m_setting.value("Common/WaringLevel").toInt();
       m_iVolume = m_setting.value("Common/Volume").toInt();
       m_iMute = m_setting.value("Common/Mute").toInt();
       m_iViewFollow = m_setting.value("Common/ViewFollow").toInt();
       m_iOverViewWidth = m_setting.value("Common/OverViewWidth").toInt();
       m_iOverViewHeath = m_setting.value("Common/OverViewHeath").toInt();
       m_iCircleViewWidth = m_setting.value("Common/CircleViewWidth").toInt();
       m_iCircleViewHeath = m_setting.value("Common/CircleViewHeath").toInt();
       m_iControlMode = m_setting.value("Common/ControlMode").toInt();
       m_strOrinIp = m_setting.value("Orin/Ip").toString();
       m_iOrinPort = m_setting.value("Orin/Port").toInt();

       m_strGpsBackgrndImg = m_setting.value("Gps/BackgroundImg").toString();
       m_strLeftTopPt = m_setting.value("Gps/LeftTopPt").toString();
       m_strRightBottomPt = m_setting.value("Gps/RightBottomPt").toString();


       m_iSafeDis = m_setting.value("Warning/Safe").toInt();
       m_iWaringDis = m_setting.value("Warning/Warning").toInt();


       m_iUseDetect = m_setting.value("Thread/Detect").toInt();
       m_iUseSgm = m_setting.value("Thread/SGM").toInt();


       m_iSimData = m_setting.value("Common/SIM").toInt();
       m_strMsgId = m_setting.value("Common/MSGID").toString();

       m_strVoictInputDevice = m_setting.value("Device/VoiceInput").toString().toStdString();
    }

    virtual bool Save()
    {
        m_setting.setValue("Common/ShowTopView",m_iShowTopView);
        m_setting.setValue("Common/ShowCircleView",m_iShowCircleView);
        m_setting.setValue("Common/WaringLevel",m_iWaringLevel);
        m_setting.setValue("Common/Volume",m_iVolume);
        m_setting.setValue("Common/Mute",m_iMute);
        m_setting.setValue("Common/ViewFollow",m_iViewFollow);
        m_setting.setValue("Common/OverViewWidth",m_iOverViewWidth);
        m_setting.setValue("Common/OverViewHeath",m_iOverViewHeath);
        m_setting.setValue("Common/CircleViewWidth",m_iCircleViewWidth);
        m_setting.setValue("Common/CircleViewHeath",m_iCircleViewHeath);
        m_setting.setValue("Common/ControlMode",m_iControlMode);


        m_setting.setValue("Thread/Detect",m_iUseDetect);
        m_setting.setValue("Thread/SGM",m_iUseSgm);

        m_setting.setValue("Common/MSGID", m_strMsgId);

        m_setting.setValue("Device/VoiceInput", QString::fromStdString(m_strVoictInputDevice));

        return CInfoMgrBase::SetServerInfo(m_strServerIp,m_iPort);
    }
public:
    int m_iShowTopView =1;
    int m_iShowCircleView = 1;
    int m_iWaringLevel =0;
    //0 -> low
    //1 -> mid
    //2 -> high
    int m_iVolume =50;
    int m_iMute = 0;
    //1 -> mute
    int m_iViewFollow = 1;
    int m_iOverViewWidth = 300;
    int m_iOverViewHeath = 300;
    int m_iCircleViewWidth = 300;
    int m_iCircleViewHeath = 300;
    int m_iControlMode = 0;
    //0 -> local
    //1 -> remote


    QString m_strOrinIp;
    int m_iOrinPort;
    QString m_strGpsBackgrndImg;
    QString m_strLeftTopPt;
    QString m_strRightBottomPt;


    //lzb add for
    int m_iSafeDis;
    int m_iWaringDis;

    // lzb add fro control thread
    int m_iUseDetect;
    int m_iUseSgm;
    int m_iSimData = 1;

    // lzb add for can message filter
    QString m_strMsgId;

    std::string m_strVoictInputDevice;
};

#endif

#ifndef QT_SETTING_HPP__
#define QT_SETTING_HPP__

#include <QSettings>
using namespace  std;
// 自己写了一个bitset的类，就简单实现功能就行了，
class CSettingMgr
{
public:
    CSettingMgr( QString strFileName )
        : m_setting(strFileName,QSettings::IniFormat)
    {
        //m_strFile = strFileName;
    }


//    std::tuple<QString, int> GetServerInfo()
//    {
//        QString strIp = m_setting.value("server/ip").toString();
//        int iPort = m_setting.value("server/port").toInt();
//        return {strIp, iPort};
//    }

//    bool SetServerInfo( QString strIp, int iPort )
//    {
//        m_setting.setValue("server/ip",strIp);
//        m_setting.setValue("server/port",iPort);
//        return true;
//    }

//    bool SetRoleInfo( char role )
//    {
//        m_setting.setValue("Client/Role",role);
//        return true;
//    }

//    char GetRoleInfo ()
//    {
//         return m_setting.value("Client/Role").toChar().toLatin1();
//    }

//private:
//    QString m_strFile;
    QSettings m_setting;
};

#endif

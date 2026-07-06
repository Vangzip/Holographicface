////////////////////////////////////////////////////////////////////////////////
//
// File Name:	qtstring.hpp
// Class Name:  CQtString
// Description:	封装一些仅在qt中使用的字符串操作
// Author:		罗志彬
// Date:		2019年4月22日
//
////////////////////////////////////////////////////////////////////////////////

#ifndef QTSTRING_H
#define QTSTRING_H

#include "CommonString.hpp"
#include <string>
#include <QUuid>
#include <QString>
#include <QtCore>
class CQtString
{
public:

    // 获得一个uuid
    static QString GetUuid()
    {
        QUuid id = QUuid::createUuid();
        QString strTmp = id.toString();

        // 去掉两边的大括号
        strTmp.remove('{');
        strTmp.remove('}');
        // 把中划线改成下划线
        for(QChar& ch : strTmp )
        {
           if( ch == '-' )
           {
               ch = '_';
           }
        }
        return strTmp;
    }

    // 获得当前程序exe所在路径
    static std::string get_app_path()
    {
        return QCoreApplication::applicationDirPath().toStdString() + "/";
    }


    static QString get_resource_path()
    {
        return get_app_path_QString() + "Resource/";
    }

    static QString get_app_path_QString()
    {
        return QCoreApplication::applicationDirPath() + "/";
    }

    // 对密码做混淆，计算MD5
    static QString MixPwdWithMD5( QString strPwd )
    {
        QCryptographicHash md( QCryptographicHash::Md5);
        QByteArray bb = QCryptographicHash::hash(strPwd.toLatin1(),QCryptographicHash::Md5);

        return QString(bb.toHex()).toUpper();
    }
    // 判断密码是否相同，预留处理接口，暂时没有实现，只判断值相等
    static bool MatchPwd(std::string strPwd, std::string value )
    {
        return strPwd == value;
    }


};

#endif // QTSTRING_H

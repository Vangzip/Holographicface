////////////////////////////////////////////////////////////////////////////////
//
// File Name:	qtfileoperate.hpp
// Class Name:  CQtFileOperate
// Description:	封装一些常用的qt文件和目录方面的操作
// Author:		罗志彬
// Date:		2019年4月22日
//
////////////////////////////////////////////////////////////////////////////////

#ifndef QTFILEOPERATE_HPP
#define QTFILEOPERATE_HPP

#include <QString>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QList>
#include <QDebug>
class CQtFileOperate
{
public:

    // 判断是否为空
    static bool isDirEmpty( QDir& dir )
    {
        // 设置遍历过滤器
        dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot |
                      QDir::Hidden );
        // 遍历文件夹
        for (QFileInfo fileinfo : dir.entryInfoList())
        {
            if( fileinfo.isDir() )
            {// 如果是
                return false;
            }
            else if( fileinfo.isFile() )
            {
                return false;
            }
            else
            {
                continue;
            }
        }
        return true;
    }

    // 删除一个文件
    static bool delFile( QFileInfo& fileinfo )
    {
        if( fileinfo.isWritable() )
        {// 如果文件可写
            // 构建操作类
            QFile file(fileinfo.absoluteFilePath() );
            // 设置写文件的权限
            file.setPermissions(QFile::WriteOwner );
        }
        // 删除文件
        return fileinfo.dir().remove(fileinfo.fileName() );
    }

    // 删除一个目录
    static void delDir( const QString strPath, bool bIncludeSelf = true )
    {
        if( strPath.isEmpty() )
        {
            return ;
        }

        QDir dir(strPath);
        // 设置遍历过滤器
        dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot |
                      QDir::Hidden );

        // 开始遍历所有的文件与文件夹
        for(QFileInfo fileInfo : dir.entryInfoList() )
        {
            if( fileInfo.isDir() )
            {// 如果是一个目录
                // 递归删除
                delDir(fileInfo.absoluteFilePath() );
            }
            else if( fileInfo.isFile() )
            {// 如果是一个文件
                delFile(fileInfo);
            }
            else
            {
                // do nothing;
            }
        }

        if( bIncludeSelf )
        {
            // 删除目录本身
            dir.rmpath(dir.absolutePath());
        }
    }

    // 创建目录
    static bool makePath( const QString& strPath )
    {
        QDir dir(strPath);
        return dir.mkpath(strPath);
    }

    // 判断一个路径名是否存在
    static bool isPathExist( const QString strPath )
    {
        QFileInfo fileInfo(strPath);
        if( fileInfo.isDir()|| fileInfo.isFile() )
        {
            return true;
        }
        return false;
    }

    // 复制文件
    static bool copyFile(QString strSrc, QString strDst, bool bForce )
    {
        //
        if( !isPathExist(strSrc) )
        {// 如果源文件不存在
            return false;
        }
        if( isPathExist(strDst)  )
        {// 如果目标文件已经存在
            if( bForce )
            {// 如果强制复制
                // 先删掉目标文件
                QFileInfo file(strDst);
                delFile(file);
            }
            else
            {
                return false;
            }
        }
        return QFile::copy(strSrc, strDst );
    }

    static std::vector<float> readMatrix( QString strFile )
    {
        std::vector<float> rs;
        QFile f(strFile);
        if( f.open(QIODevice::ReadOnly) )
        {
            auto lst = f.readAll().split(',');
            if( lst.size() != 16 )
            {
                qDebug()<<"Need 16 numbers, but file " << strFile << " has " << lst.size();

            }
            else
            {
                for( auto str : lst )
                {
                    rs.push_back(str.toDouble());
                }
            }

            f.close();
        }
        return rs;
    }

    static QList<QFileInfo> enumFiles(QString strDir, QStringList lstNameFilter ={}, bool bCascade = false )
    {
        QList<QFileInfo> rt;
        QDir dir(strDir);
        // 设置遍历过滤器
        dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot |
                      QDir::Hidden );
        if( !lstNameFilter.empty() )
        {
            dir.setNameFilters(lstNameFilter);
        }

        // 开始遍历所有的文件与文件夹
        for(QFileInfo fileInfo : dir.entryInfoList() )
        {
            if( fileInfo.isDir() )
            {
                if( bCascade )
                {
                    auto child = enumFiles(fileInfo.absolutePath(),lstNameFilter,bCascade );
                    for( auto& pt : child )
                    {
                        rt.push_back(pt);
                    }
                }
            }
            else if( fileInfo.isFile() )
            {
                rt.push_back(fileInfo);
            }
        }
        return rt;
    }
    static std::vector<QByteArray> getFile(std::string strPath )
    {
        std::vector<QByteArray> rs;

        QFile f(QString::fromStdString(strPath));
        if( !f.open(QIODevice::ReadOnly ) )
        {
            return rs ;
        }

        while( true )
        {
            auto l = f.readLine();
            if( !l.size() )
            {
                break;
            }
            rs.push_back(l);
        }

        f.close();
        return rs ;
    }
};

#endif // QTFILEOPERATE_HPP

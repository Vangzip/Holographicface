////////////////////////////////////////////////////////////////////////////////
//
// File Name:	DbInterface.h
// Class Name:  CDbInterface
// Description:	基础数据库操作接口类，直接操作数据库文件，
// Author:		罗志彬
// Date:		2019年4月22日
//
////////////////////////////////////////////////////////////////////////////////

#ifndef DBINTERFACE_HPP
#define DBINTERFACE_HPP

#include <string>
#include <vector>
#include <functional>
#include <memory>

class CEntityBase;

class CDbInterface
{
public:
    CDbInterface()
	{
		
	}
    virtual ~ CDbInterface()
    {

    }

///////////////// 纯虚函数接口声明，在派生类中实现
public:
    // 打开数据库连接
    virtual bool Connect( const std::string& ) = 0;
    // 断开Sqlite数据库
    virtual bool Close() =0;

    // 查询符合条件的数据
    virtual bool select(const std::string& strSql,
                        std::function<CEntityBase*()> funcCreater,
                        std::vector<CEntityBase*>& vOut )= 0;

    // 查询符合条件一条记录，如果查询到多条，就返回第一个
    virtual bool select_single( const std::string& strSql, CEntityBase* pEntity ) = 0;
    // 执行返回单一结果的查询
    virtual std::string select(const std::string& strSql ) = 0;

    // 插入修改删除
    virtual bool Insert(std::string strInsert) = 0;
    virtual bool Update(std::string strUpdate) = 0;
    virtual bool Delete(std::string strDelete) = 0;

    // 事务控制，都实现了，就是没使用
    virtual bool begin( )   =0;
    virtual bool rollback( )=0;
    virtual bool commit( )  =0;

    // 判断数据库是否打开
    virtual bool isDbOpen() = 0;


    virtual int execSql( std::string strSql ) = 0;

    virtual bool saveBlobExec(std::string strSql, std::vector<unsigned char> vctData )=0;

/////////////// 功能函数，
public:

    virtual std::string get_uuid()
    {
        return "";
    }

///////////// 内联函数
public:
    // 获得最后一条出错信息
    inline std::string GetLastError()
    {
        return m_strLastError;
    }
/////////////// 成员变量
protected:
    std::string m_strLastError;
};

#endif // DBINTERFACE_HPP

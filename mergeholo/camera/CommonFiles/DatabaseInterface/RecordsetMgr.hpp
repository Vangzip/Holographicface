#ifndef RECORDSETMGR_H
#define RECORDSETMGR_H

#include <string>
#include <vector> // 为了使用std::vector

// 如果没有定义__int64，则将其定义为long long类型
#ifndef __int64
#define __int64 long long
#endif

// CRecordsetMgr类的声明
class CRecordsetMgr
{
public:
    CRecordsetMgr(){} // 构造函数
    virtual ~CRecordsetMgr(){} // 虚析构函数
public:

    /**
     * @brief 获取int类型的值
     * @param iColIndex 列索引
     * @param val 保存值的引用
     * @return 获取成功返回true，否则返回false
     */
    inline virtual bool get_value( int iColIndex, int& val ) = 0;

    /**
     * @brief 获取double类型的值
     * @param iColIndex 列索引
     * @param val 保存值的引用
     * @return 获取成功返回true，否则返回false
     */
    inline virtual bool get_value( int iColIndex, double& val ) = 0;

    /**
     * @brief 获取bool类型的值
     * @param iColIndex 列索引
     * @param val 保存值的引用
     * @return 获取成功返回true，否则返回false
     */
    inline virtual bool get_value( int iColIndex, bool& val ) = 0;

    /**
     * @brief 获取std::string类型的值
     * @param iColIndex 列索引
     * @param val 保存值的引用
     * @return 获取成功返回true，否则返回false
     */
    inline virtual bool get_value( int iColIndex, std::string& val ) = 0;

    /**
     * @brief 获取__int64类型的值
     * @param iColIndex 列索引
     * @param val 保存值的引用
     * @return 获取成功返回true，否则返回false
     */
    inline virtual bool get_value( int iColIndex, __int64& val ) = 0;

    /**
     * @brief 获取char类型的值
     * @param iColIndex 列索引
     * @param ch 保存值的引用
     * @return 获取成功返回true，否则返回false
     */
    inline virtual bool get_value( int iColIndex, char& ch ) = 0;

    /**
     * @brief 获取short类型的值
     * @param iColIndex 列索引
     * @param val 保存值的引用
     * @return 获取成功返回true，否则返回false
     */
    inline virtual bool get_value( int iColIndex, short& val) = 0;

    /**
     * @brief 获取std::vector<unsigned char>类型的值
     * @param iColIndex 列索引
     * @param val 保存值的引用
     * @return 获取成功返回true，否则返回false
     */
    inline virtual bool get_value( int iColIndex, std::vector<unsigned char>& val) = 0;
};

#endif // RECORDSETMGR_H
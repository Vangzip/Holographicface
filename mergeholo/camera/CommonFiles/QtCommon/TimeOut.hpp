//********************************************************************
//* 文件名称： TimeOut.hpp
//* 文件功能： 计时器类
//* 创 建 人： 罗志彬
//* 创建时间： 2019年1月28日
//********************************************************************
#ifndef CTIME_OUT_HPP
#define CTIME_OUT_HPP
#include <QDateTime>
#include <functional>
#include <QThread>

/// @brief CTimeout 类用于处理超时逻辑
class CTimeout
{
public:
    /// @brief 构造函数，默认构造时间为0
    /// @param llSecond 秒
    /// @param llMSecond 毫秒
    CTimeout(long long llSecond = 0, int llMSecond = 0)
    {
        // 获取当前时间的毫秒表示
        m_llBegin = QDateTime::currentDateTime().toMSecsSinceEpoch();
        // 设置超时时间（转换为毫秒）
        m_llTimeout = llSecond * 1000 + llMSecond;
    }

    /// @brief 判断是否超时
    /// @return 是否超时
    bool isTimeout()
    {
        if (!m_llTimeout)
        { // 如果没设置超时时间，就一直卡住
            return false;
        }
        long long llNow = QDateTime::currentDateTime().toMSecsSinceEpoch();
        return llNow > (m_llTimeout + m_llBegin);
    }

    /// @brief 获取从开始计时到当前所经历的秒数
    /// @return 经历秒数
    long long secondSinceBegin()
    {
        return milisecondSinceBegin() / 1000;
    }

    /// @brief 获取从开始计时到当前所经历的毫秒数
    /// @return 经历毫秒数
    long long milisecondSinceBegin()
    {
        long long llNow = QDateTime::currentDateTime().toMSecsSinceEpoch();
        return llNow - m_llBegin;
    }

public:
    long long m_llTimeout; ///< 超时时间（毫秒）
    long long m_llBegin;   ///< 开始计时时间（毫秒）
};

/// @brief 模板类CWaitCondition用于在指定时间内等待某条件成立
/// @tparam timeout 超时时间（秒）
template <int timeout = 0>
class CWaitCondition
{
public:
    /// @brief 操作符重载，用来执行传入的条件函数
    /// @param funcCondition 传入的条件函数
    /// @return 条件是否满足
    bool operator()(std::function<bool()> funcCondition)
    {
        // 创建CTimeout对象，用于记录等待时间
        CTimeout tmot(timeout);
        // 循环等待，直到条件满足或超时
        while (!funcCondition() && !tmot.isTimeout())
        {                          // 条件谓词不满足，并且线程没结束，
            QThread::msleep(1);    // 休眠1毫秒，避免过多占用CPU
            qApp->processEvents(); // 处理Qt应用程序的事件队列
        }
        // 返回值是是否超时，方便区分是条件满足退出的，还是超时退出的
        return !tmot.isTimeout();
    }
};

/// @brief 模板类COnlyWait仅等待指定时间
/// @tparam timeout 超时时间（秒）
template <int timeout = 0>
class COnlyWait : public CWaitCondition<timeout>
{
public:
    /// @brief 操作符重载，等待指定时间，条件永远为false
    /// @return 条件是否满足
    bool operator()()
    {
        return CWaitCondition<timeout>::operator()(
            []() -> bool
            { return false; });
    }
};

using CWait30S = CWaitCondition<30>;    ///< 等待30秒
using CWait10S = CWaitCondition<10>;    ///< 等待10秒
using CWaitForever = CWaitCondition<0>; ///< 永远等待
using CWaitNone = CWaitCondition<-1>;   ///< 不等待

#endif
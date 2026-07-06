 #ifndef JP_CTIME_HPP
#define JP_CTIME_HPP
#include <iostream>
#include <string>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <type_traits>
#include <functional>

namespace JP
{
    // 时间类型萃取前置sheng'mign声明
    template <typename period_t>
    struct period_traits;

    /// 定义类型别名
    using duration_t = std::chrono::nanoseconds;
    using timestamp_t = std::chrono::time_point<std::chrono::system_clock, duration_t>;
    static_assert(std::is_same<int64_t, duration_t::rep>::value,
                  "The underlying type of the microseconds should be int64.");
    // 纳秒
    using nanos_t = std::chrono::nanoseconds;
    // 微秒
    using micro_t = std::chrono::microseconds;
    // 毫秒
    using milli_t = std::chrono::milliseconds;
    using millis = milli_t;// 兼容旧代码
    // 秒
    using second_t = std::chrono::seconds;
    // 分钟
    using minute_t = std::chrono::minutes;
    // 小时
    using hour_t = std::chrono::hours;

    /////
    ///类型萃取，主要提供格式化方式
    ///
    template<>
    struct period_traits<milli_t>
    {
        static inline std::string name()
        {
            return "ms";
        }
        //inline static const std::string name="ms";
        static inline std::string format( milli_t::rep time )
        {
            return std::to_string(time) +" "+ name();
        }
    };

    template<>
    struct period_traits<micro_t>
    {
        static inline std::string name()
        {
            return "us";
        }
        static inline std::string format( micro_t::rep time )
        {
            return std::to_string(time) +" "+ name();
        }
    };

    template<>
    struct period_traits<second_t>
    {
        static inline std::string name()
        {
            return "s";
        }
        static inline std::string format( second_t::rep time )
        {
            return std::to_string(time) +" "+ name();
        }
    };

    // 浮点型毫秒值，方便使用
    template<>
    struct period_traits< std::chrono::duration<double, std::ratio<1,1000>>>
    {
        static inline std::string name()
        {
            return "ms";
        }
        static inline std::string format( double time )
        {
            return std::to_string(time) +" "+ name();
        }
    };

    // 基类
    template< typename period_t = std::chrono::duration<double, std::ratio<1,1000>> >
    class CJpCTime
    {
        // 时间单位类型
        using time_type = typename period_t::rep;
        // 自定义格式化函数类型
        using formaterType = std::function<void(std::string, time_type)>;
    public:
        // 构造和析构
        CJpCTime( std:: string title, formaterType func = nullptr )
            :m_strTitle(title),m_pfuncFormater(func)
        {
            t_s_ = std::chrono::steady_clock::now();
        }
        ~CJpCTime()
        {
            // 析构时也打印一下时间差
            print("Destruct");
        }

        // 返回当前时间差
        time_type GetVal()
        {
            t_e_ = std::chrono::steady_clock::now();
            time_type useTime = std::chrono::duration_cast<period_t>(t_e_ - t_s_).count();
            return useTime;
        }

        // 返回其他类型的值
        template <typename return_type >
        return_type GetVal()
        {
            return static_cast<return_type>(GetVal());
        }


        static double GetNow()
        {
            auto tp = std::chrono::steady_clock::now();
            double val = tp.time_since_epoch().count() / 1000000000.0;
            return val;
        }

        // 打印从重置或者构造开始，到现在的时间差
        void printAlltime( std::string strNote, bool bReset = true )
        {
            // 计算时间差
            time_type useTime = GetVal();
            std::cout << m_strTitle << ": " <<strNote <<" " << period_traits<period_t>::format(useTime) << std::endl;

            if( bReset )
            {
                reset();
            }
        }
        void print( )
        {
            // 计算时间差
            time_type useTime = GetVal();
            if(m_pfuncFormater )
            {// 如果定义了【自定义格式化函数，就调用】
                m_pfuncFormater(m_strTitle,useTime);
            }
            else
            {// 否则，提取对应类型的格式化方式并输出，
#ifdef ENABLE_OUTPUT
                std::cout << m_strTitle << ": " << period_traits<period_t>::format(useTime) << std::endl;
#endif
            }
        }

        inline void print( std::string strNote, bool bReset = true )
        {
            // 计算时间差
            time_type useTime = GetVal();
            #ifdef ENABLE_OUTPUT
            if(m_pfuncFormater )
            {// 如果定义了【自定义格式化函数，就调用】
                m_pfuncFormater(m_strTitle,useTime);
            }
            else
            {// 否则，提取对应类型的格式化方式并输出，

                std::cout << m_strTitle << ": " <<strNote <<" " << period_traits<period_t>::format(useTime) << std::endl;

            }
            if( bReset )
            {
                reset();
            }
            #endif
        }

        // 重置
        void reset()
        {
            t_s_ = std::chrono::steady_clock::now();
        }
    private:
        std::string m_strTitle;
        formaterType m_pfuncFormater;
        std::chrono::steady_clock::time_point t_s_; //start time ponit
        std::chrono::steady_clock::time_point t_e_; //stop time point
    };

    // 快速使用类型别名
    // 毫秒
    using CJpCTimeMs = CJpCTime<milli_t>;
    // 微秒
    using CJpCTimeUs = CJpCTime<micro_t>;
    // 秒
    using CJpCTimeSecond = CJpCTime<second_t>;
    // 浮点型毫秒
    using CJpCTimeFloatMs = CJpCTime<std::chrono::duration<double, std::ratio<1,1000>>>;




   
};

// 兼容旧代码
using JpCtime = JP::CJpCTimeMs;

#endif

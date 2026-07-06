////////////////////////////////////////////////////////////////////////////////
//
// File Name:	NewTec.hpp
// Class Name:
// Description:	手动实现一些c++新标准中提供但是c17之前的标准里没有的技术
// Author:		罗志彬
// Date:		2019年4月22日
//
////////////////////////////////////////////////////////////////////////////////

#ifndef NewTec_HPP
#define NewTec_HPP

#include <iostream>

// 以下的东西，是C++20标准中std::is_detected算子的实现，
// 使用SFINAE来检测任意成员函数是否存在，不受参数限制
namespace
{
    // 无此函数
    struct nonesuch
    {
        nonesuch() = delete;
        ~nonesuch() = delete;
        nonesuch( const nonesuch& ) = delete;
        void operator=( const nonesuch& ) = delete;
    };

    // SFINAE匹配失败
    template<class Default, class AlwaysVoid,
             template<class...> class Op, class... Args>
    struct detector
    {
        using value_t = std::false_type;
        using type = Default;
    };

    // SFINAE匹配成功
    template<class Default, template<class ...> class Op, class... Args>
    struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
    {
        using value_t = std::true_type;
        using type = Op<Args...>;
    };

    // 实现is_detected算子
    template<template<class...> class Op, class... Args >
    using is_detected = typename detector<nonesuch, void ,Op, Args...>::value_t;

    // 获得函数类型
    template<template<class...> class Op, class... Args >
    using detected_t = typename detector<nonesuch, void ,Op,Args...> :: type;
};



namespace MetaFunction
{

/// 计算一个类是否为模板类的实例
///
///

    // 基础类型，结果为否
    template<  template <class...> class Target, class T >
    struct is_template_of : public std::false_type{};

    // 偏特化类型，当第二参数为第一参数的实例化时，使用true_type
    template<  template <class...> class Target, class ... Args >
    struct is_template_of<Target, Target<Args...> > : public std::true_type {};

    // 编译期常量，避免多次计算，is_template_of_v
    template<  template <class...> class Target, class ... Args >
    inline constexpr bool is_template_of_v = is_template_of<Target,Args...>::value;


/// 用指定的算子operator_type 来判断变参列表中的所有类型是否都符合条件
///
///
    // 迭代终点，如果迭代到最后一步，说明所有的类型都符合条件
    template <template <typename T> class operator_type,
              typename ... Args >
    struct is_all_of : public std::true_type{};

    // 偏特化 迭代变参，
    template <template <typename T> class operator_type,
              typename  _This, // 当前类型
              typename ... _Rest // 剩余类型
              >
    struct is_all_of<operator_type,_This,_Rest...> : public  std::conditional<
            !operator_type<_This>::value,       // 判断当前类型是否满足条件
            std::false_type,                    // 如果不满足，整体继承false_type，退出迭代
            is_all_of<operator_type,_Rest...>   // 如果满足，继续迭代下一个
            >::type {};

    // 元变量
    template <template <typename T> class operator_type,
              typename ... Args >
    inline constexpr bool is_all_of_v = is_all_of<operator_type, Args...>::value;


/// 用指定的算子operator_type 来判断变参列表中是否至少有一个符合条件
///
///
    // 迭代终点，如果迭代到最后一步，说明所有的类型都不符合条件
    template <template <typename T> class operator_type,
        typename ... Args >
    struct is_one_of : public std::false_type{};

    // 偏特化 迭代变参，
    template <template <typename T> class operator_type,
              typename  _This, // 当前类型
              typename ... _Rest // 剩余类型
              >
    struct is_one_of<operator_type,_This,_Rest...> : public  std::conditional<
            operator_type<_This>::value,        // 判断当前类型是否满足条件
            std::true_type,                     // 如果满足，整体继承true_type，退出迭代
            is_one_of<operator_type,_Rest...>   // 如果不满足，继续迭代下一个
            >::type {};

    // 元变量
    template <template <typename T> class operator_type,
              typename ... Args >
    inline constexpr bool is_one_of_v = is_one_of<operator_type, Args...>::value;

    // 元变量 所有变参都不满足条件 是 【至少一个变参满足条件的逆命题】
    template <template <typename T> class operator_type,
              typename ... Args >
    inline constexpr bool is_none_of_v = !( is_one_of<operator_type, Args...>::value );

};




// 声明元函数， 用来判断一个类中是否有某一个成员函数
#define DECLARE_CLASS_HAS_MEMBERFUNC( class_name, function_name )\
struct class_name##_has_func_##function_name\
{\
    template<class T, typename... Args > \
    using class_name##has_##function_name##_t = decltype(std::declval<T>().function_name(std::declval<Args>()...)); \
    template<typename T, typename ... Args > \
    using class_name##has##function_name = is_detected<class_name##has_##function_name##_t,T,Args...>;\
    enum {value = class_name##has##function_name<class_name>::value };\
};

#define TEMPLATE_DECLARE_CHECK_MEMBER_FUNCTION( function_name )\
template <typename class_name, typename ... function_args >\
struct has_member_function_##function_name\
{\
    template<class T, typename... Args > \
    using has_##function_name##_t = decltype(std::declval<T>().function_name(std::declval<Args>()...)); \
    template<typename T, typename ... Args > \
    using has_##function_name = is_detected<has_##function_name##_t,T,Args...>;\
    enum { value = has_##function_name<class_name,function_args...>::value };\
};


// 检查 xxxx 是否是类T的成员
#define TEPLATE_DECLARE_CHECK_MEMBER_VARIABLE( member_name ) \
template <typename T>\
struct has_member_##member_name\
{\
    /* 如果 &T::xxxx 合法, 本函数就合法\
     下面check<T>(0)优先调用本函数, 返回值是void*/\
    template<typename U>\
    static void check(decltype(&U::member_name));\
    /* 否则, 下面chech<T>(0)调用本函数\
       返回值是int*/\
    template<typename U>\
    static int check(...);\
    /* 如果check<T>(0)的返回值是void,\
       则&T::xxxx合法, xxxx是T的成员*/\
    enum { value = std::is_void<decltype(check<T>(nullptr))>::value };\
};

#endif // NewTec_HPP

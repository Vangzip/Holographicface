////////////////////////////////////////////////////////////////////////////////
//
// File Name:	CommonAlgorithm.hpp
// Class Name:	CCommonAlgorithm
// Description:	通用基础算法操作，包括查找，遍历等
// Author:		罗志彬
// Date:		2018年11月9日
//
////////////////////////////////////////////////////////////////////////////////

#ifndef COMMON_ALGORITHM_HPP
#define COMMON_ALGORITHM_HPP

#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <stdarg.h>
#include <list>
#include <functional>
using namespace std;


class CCommonAlgorithm
{
public:

	// 变参模板函数  
	// 使用funcCall对变参部分进行逐一处理
	// icnt 变参个数 funcCall 回调函数 后续为参数真值
	template<typename T>
	static std::vector<int> deal_each( int iCnt, std::function<bool(int iIndex, T& item)> funcCall, ... )
	{
		// 初始化返回结构体
		std::vector<int> vctRs;

		// 初始化变参列表
		va_list ap;

		// 定位参数
		va_start(ap, funcCall);

		// 调用同名模板机型处理
		vctRs = deal_each<T>(iCnt, funcCall, ap);
		// 结束变参列表
		va_end(ap);
		// 返回结果
		return vctRs;
	}

	// 变参模板函数  
	// 使用funcCall对变参部分进行逐一处理
	// icnt 变参个数 funcCall 回调函数 后续为参数真值
	template<typename T>
	static std::vector<int> deal_each( int iCnt, std::function<bool(int iIndex, T& item)> funcCall, va_list ap )
	{

		// 初始化返回结构体
		std::vector<int> vctRs;
		// 遍历变参列表
		for( int i=0; i<iCnt; i++ )
		{
			// 获得参数
            T item = va_arg(ap, T);
			// 调用回调函数进行处理
			if( funcCall(i,item) )
			{// 如果成功，就计入一个0
				vctRs.push_back(0);
			}
			else
			{//  如果失败，计入-1作为错误
				vctRs.push_back(-1);
			}
		}
		// 返回结果
		return vctRs;
	}


	template< typename data_type, 
		template< typename container_data_type, typename al = std::allocator<container_data_type> > class container_type 
	>
	static bool deal_each( std::function<container_type<data_type>() > funcGetter,// 获得容器数据的方法
	std::function<bool(data_type)>  funcDeal,// 处理容器数据的方法
	std::function<bool()> funcBefore = nullptr, // 处理之前的前置函数
	std::function<bool(container_type<data_type>&)> funcAfter = nullptr) // 处理之后的后置函数
	{
		// 函数指针是否有效
		if( !funcGetter || !funcDeal )
		{
			return false;
		}

		// 先决处理
		if( funcBefore && !funcBefore() )
		{
			return false;
		}

		bool bIs = true;

		// 获得容器数据
		container_type<data_type> container = funcGetter();

		for(auto it = container.begin(); it!= container.end(); it ++ )
		{
			bIs &= funcDeal(*it);
		}


		// 后置处理
		if( funcAfter && !funcAfter(container) )
		{
			return false;
		}

		return bIs;
	}


	//用来将dwMask 从低到高 第iIndex 个二进制位 设置成为 bTrue
	static bool SetBit(unsigned int* pdwMask,  int iIndex, bool bTrue)
	{
		if( bTrue )
		{// 如果置为一， 移位之后按位或
			*pdwMask |= ( (0x00000001) << iIndex);
		}
		else
		{// 如果置为0， 移位取反之后按位与
			*pdwMask &= (~(0x00000001 << iIndex) );
		}
		return true;
	}



	// 构造一个掩码，变参为每一位要设置成什么, 从右到左
    inline static unsigned int MakeMask( int iCnt, ... )
	{
		va_list ap;
		va_start(ap,iCnt);
		unsigned int dwMask = 0;

		// 将SetBit函数第一个参数绑定到dwMask
        auto funcSetBit = std::bind(&CCommonAlgorithm::SetBit,&dwMask, std::placeholders::_1,std::placeholders::_2);

		// 调用通用算法进行处理
		CCommonAlgorithm::deal_each<bool>(iCnt, funcSetBit , ap);
		va_end(ap);
		return dwMask;
	}

	// 检测掩码的某一位是否为1
    inline static bool CheckBit( unsigned int dwMask, int iBit )
	{
		return !!(dwMask & (0x00000001<<iBit) );
	}

	// 浮点数相等比较
    template<typename left_type, typename right_type >
	static bool IsFloatEqual( left_type l, right_type r )
	{
		// 浮点数最小精度差
		const double cdMin = 0.00000001;
		// 得到两个数的差值 
		double dRs = fabs( l - r );
		// 返回比较结果
		return dRs < cdMin;
	}


    template<	typename data_type,  //第一模板参数，容器的数据类型
            // 第二模板参数，一个以数据类型和allocator为模板的容器，适配stl容器，，默认使用std::allocator
            template< typename container_data_type, typename al = std::allocator<container_data_type> >
                class container_type = std::vector
        >
    inline static void clear( container_type<data_type*>& container )
    {
        for(auto it = container.begin();
            it != container.end();
            it ++ )
        {
            if( *it )
            {
                delete (*it);
            }
        }
        container.clear();
    }

    // 判断容器中是否存在指定的值,需要提供比较算子
    template <typename value_type, typename data_type = value_type, template < class ... T> class container_type>
    static bool IsContain( value_type&& value, const container_type<data_type>& container,
                            std::function<bool(value_type,data_type)> funcComp
                            )
    {
        for( auto& item : container )
        {
            if( funcComp(value,item ) )
            {
                return true;
            }
        }
        return false;
    }

    template <  typename data_type , template < class ... T> class container_type>
    static bool IsContain( const container_type<data_type>& container,
                            std::function<bool(data_type)> funcComp
                            )
    {
        for( auto item : container )
        {
            if( funcComp( item ) )
            {
                return true;
            }
        }
        return false;
    }

    // 判断容器中是否存在指定的值，为 重载了==操作符的类型提供
//    template <typename value_type, typename data_type = value_type, template < class ... T> class container_type>
//    static bool IsContain( value_type&& value, const container_type<data_type>& container)
//    {
//        for( auto& item : container )
//        {
//            if( item == value )
//            {
//                return true;
//            }
//        }
//        return false;
//    }

    template <typename value_type, typename data_type = value_type, template < class ... T> class container_type>
    static bool IsContain( value_type value, const container_type<data_type>& container)
    {
        for( auto& item : container )
        {
            if( item == value )
            {
                return true;
            }
        }
        return false;
    }


    // 在一个容器里查找符合条件的第一条数据
    template<typename data_type, template < class ... T> class container_type,
            typename std::enable_if<std::is_pointer<data_type>::value>::type* = nullptr >
    static data_type find( container_type<data_type>& container,
                                                      std::function<bool(data_type&)> funcComp )
    {
        auto item = std::find_if(container.begin(), container.end(), funcComp );
        if( item == container.end() )
        {
            return nullptr;
        }
        return *item;
    }

    template<typename data_type, template < class ... T> class container_type,
            typename std::enable_if<!std::is_pointer<data_type>::value>::type* = nullptr >
    static data_type find( container_type<data_type>& container,
                                                      std::function<bool(data_type&)> funcComp )
    {
        auto item = std::find_if(container.begin(), container.end(), funcComp );
        if( item == container.end() )
        {
            return data_type();
        }
        return *item;
    }


    // 使用默认比较运算符进行比较的函数
    template<typename l_type, typename r_type >
    static bool is_same( l_type&& l ,r_type&& r )
    {
        return l == r;
    }

    // 判断一个数据，是否为后面的变参之中的一个
    template<typename value_type, class ... arg_types >
    static bool is_one_of ( value_type value, arg_types... arg )
    {
        // 依次判断数值是否为所有变参中的一个
        auto lst = {is_same(value, std::forward<arg_types>(arg))...};
        // 如果所有的结果中有true，那就意味着是找到了，否则就是没找到
        return IsContain<bool>(true,lst);
    }


private:

};


using Algo = CCommonAlgorithm;

#endif //COMMON_STRING_HPP

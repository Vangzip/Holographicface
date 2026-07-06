////////////////////////////////////////////////////////////////////////////////
//
// File Name:	CommonString.hpp
// Class Name:  CCommonString
// Description:	公用的字符串操作函数
// Author:		罗志彬
// Date:		2019年4月22日
//
////////////////////////////////////////////////////////////////////////////////
#ifndef COMMONSTRING_HPP
#define COMMONSTRING_HPP
#include <string>
#include <functional>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <stdarg.h>
#include "CommonAlgorithm.hpp"
class CCommonString
{

public:

    // 模板函数，用来将容器中的数据使用 strSeperator作为分隔符连接在一起
        // 模板参数1 容器数据类型
        // 模板参数2 一个以数据类型和allocator为模板的容器，适配stl容器，，默认使用std::allocator
        // 备注：函数也可以声明为如下类型
        //template <typename data_type,
        //	template < class ... T> class container_type
        //>
        // 使用这样的声明，可以适配更多的容器，但是vs2010貌似还不支持可变模板参数，待技术更新可实现之
        template<	typename data_type,  //第一模板参数，容器的数据类型

            // 第二模板参数，一个以数据类型和allocator为模板的容器，适配stl容器，，默认使用std::allocator
            template< typename container_data_type, typename al = std::allocator<container_data_type> > class container_type
        >
        static std::string joinString(	container_type<data_type>& container, //参数 容器
        std::string strSeperator, // 参数 分隔符
        bool bExceptEmpty,	// 参数 是否排除空字符串
        std::function<std::string(typename container_type<data_type>::iterator it)> funcGetString // 参数 将data_type 转成string的函数
        )
        {
            //初始化返回结果
            std::string strRs = "";
            //初始化迭代器
            typename container_type<data_type>::iterator it = container.begin();

            for( ; it!= container.end(); it ++ )
            {// 遍历容器
                if( funcGetString )
                {// 如果转换指针存在
                    // 获得字符串值
                    std::string strValue = funcGetString(it);
                    if( strValue.empty() && bExceptEmpty )
                    {// 排除空串
                        continue;
                    }

                    // 在字符串后面接上新串以及分隔符
                    strRs+= (strValue + strSeperator);
                }
            }

            // 由于每一个后面都加了分隔符，最后就多了一个，所以得删掉
            if( !strRs.empty() )
            {
                strRs.pop_back();
            }
            return strRs;
        }

        // 模板函数的偏特化版本，第一参数使用string, 因为模板函数不允许指定默认参数，这样写能简单点
        template<
            // 模板参数，一个以数据类型和allocator为模板的容器，适配stl容器，，默认使用std::allocator
            template< typename container_data_type, typename al = std::allocator<container_data_type> > class container_type
        >
        static std::string joinString(container_type<std::string>& container, //参数 容器
        std::string strSeperator, // 参数 分隔符
        bool bExceptEmpty = true,	// 参数 是否排除空字符串
        std::function<std::string(typename container_type<std::string>::iterator it)> funcGetString = nullptr// 参数 将datatype 转成string的函数
        )
        {
            //初始化返回结果
            std::string strRs = "";
            //初始化迭代器
            typename container_type<std::string>::iterator it = container.begin();

            for (; it != container.end(); it++)
            {// 遍历容器
                std::string strValue = "";

                if (funcGetString)
                {// 如果转换指针存在,说明用户提供了额外的获取字符串函数
                    // 获得字符串值
                    strValue = funcGetString(it);
                }
                else
                {// 如果没提供特殊的函数
                    //偏特化版本直接操作迭代器就可以了
                    strValue = (*it);
                }

                if( strValue.empty() && bExceptEmpty )
                {// 排除空串
                    continue;
                }

                // 在字符串后面接上新串以及分隔符
                strRs+= (strValue + strSeperator);
            }

            // 由于每一个后面都加了分隔符，最后就多了一个，所以得删掉
            if( !strRs.empty() )
            {
                strRs.pop_back();
            }
            return strRs;
        }
    template<typename string_type >
    static string_type joinString( string_type strSperator, int iCnt, ... )
    {
        string_type rs;

        // 初始化变参列表
        va_list ap;

        // 定位参数
        va_start(ap, iCnt);

        // 调用处理
        CCommonAlgorithm::deal_each<string_type>(iCnt, [&](int iIndex, string_type& item)->bool
        {
            // mod by wenqiang. 2019-3-21. Re: 修改一下
            //if( 0!= iCnt )
            if (0 != iIndex)
            {// 在字符串后面接上新串以及分隔符
                rs += strSperator;
            }

            rs += item;
            return true;
        }
        , ap);

        return rs;
    }

    template<typename data_type>
    static std::string joinString( std::string strSeperator, bool bExceptEmpty,
                                   std::function<std::string(data_type&)> funcToString , int iCnt,  ...)
    {
        std::string strRs = "";

        // 初始化变参列表
        va_list ap;

        // 定位参数
        va_start(ap, iCnt);

        // 调用处理
        CCommonAlgorithm::deal_each<data_type>(iCnt, [&](int iIndex, data_type& item)->bool
        {
            if( !funcToString )
            {// 转换函数不存在，返回false
                return false;
            }
            string strValue = funcToString(item);
            if( strValue.empty() && bExceptEmpty )
            {// 空值，不追加
                return false;
            }

            // 在字符串后面接上新串以及分隔符
            strRs+= (strValue + strSeperator);
            return true;
        }
        , ap);
        // 结束变参列表
        va_end(ap);
        // 返回结果

        // 由于每一个后面都加了分隔符，最后就多了一个，所以得删掉
        if( !strRs.empty() )
        {
            strRs.pop_back();
        }

        return strRs;
    }


    template<typename data_type>
    static std::string joinString( std::string strSeperator, bool bExceptEmpty, int iCnt,  ...)
    {
        std::string strRs = "";

        // 初始化变参列表
        va_list ap;

        // 定位参数
        va_start(ap, iCnt);

        // 调用处理
        CCommonAlgorithm::deal_each<data_type>(iCnt, [&](int iIndex, data_type& item)->bool
        {

            if( item.empty() && bExceptEmpty )
            {// 空值，不追加
                return false;
            }

            //if( 0!= iCnt )
            if (0 != iIndex)
            {// 在字符串后面接上新串以及分隔符
                strRs += strSeperator;
            }

            strRs += item;
            return true;

        }
        , ap);
        // 结束变参列表
        va_end(ap);
        // 返回结果

//        // 由于每一个后面都加了分隔符，最后就多了一个，所以得删掉
//        if( !strRs.empty() )
//        {
//            strRs.pop_back();
//        }

        return strRs;
    }

    // 将一个字符串用一个 标签围起来
    inline static std::string SurroundWidh( std::string& strIn, std::string strTag = "\"" )
    {
        return strTag + strIn + strTag;
    }


	inline static std::string get_app_path()
	{
		return "";
	}
	
	static std::string trimLeft( string strIn )
	{	
		std::string strOut = strIn;
		for(auto it  = strOut.begin(); it!= strOut.end(); it++ )
		{
			if( (*it) == ' ' )
			{
				it = strIn.erase(it);
			}
		}
		return strOut;
	}

	static std::string trimRight( string strIn )
	{
		string strOut = strIn;
		
		while( strOut.back() == ' ' || strOut.back() == '\0'  || strOut.back() == '\n'  )
		{
			strOut.pop_back();
		}
		return strOut;
	}

	static std::string trim( string strIn )
	{
		return trimLeft( trimRight(strIn) );
	}
	
	
	static std::vector<string> SplitToVector( string strContent, char ccSp )
	{
		std::vector<string> vctRs;
		std::stringstream ss(strContent);
		string strSubStr;
		while( std::getline(ss,strSubStr,ccSp) )
		{
			//MakeUpper(strSubStr);
			vctRs.push_back(strSubStr);
		}
		return vctRs;
	}

	static void MakeUpper( string& str)
	{
		std::transform(str.begin(),str.end(),str.begin(),::toupper);
	}
	static void MakeLower( string& str)
	{
		std::transform(str.begin(),str.end(),str.begin(),::tolower);
	}
 
    template< typename ... arg >
    static std::string sprintf( const char* fmt, arg... values )
    {

   
        size_t num_of_chars = static_cast<size_t>(std::vsnprintf(nullptr,0,fmt,values...));
        std::string sr(num_of_chars+1,'\0');
      
        std::vsnprintf(const_cast<char*>(sr.data()),sr.size(),fmt,values...);
        sr.resize(num_of_chars);
        return sr;
    }

	    //格式化字符串
    static std::string Format(const char* fmt...)
    {
        std::string sr;
        va_list args0,args1;
        va_start(args0,fmt);
        va_copy(args1,args0);
        size_t num_of_chars = static_cast<size_t>(std::vsnprintf(nullptr,0,fmt,args0));
        va_end(args0);

        sr.resize(num_of_chars+1,'\0');
        std::vsnprintf(const_cast<char*>(sr.data()),sr.size(),fmt,args1);
        sr.resize(num_of_chars);

        va_end(args1);
        return sr;

    }
 
 };

#endif // COMMONSTRING_HPP

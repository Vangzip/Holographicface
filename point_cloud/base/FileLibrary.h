///////////////////////////////////////////////////////////
//  FileLibrary.h
//  Implementation of the Class FileLibrary
//  Created on:      11-XX-2017 17:40:43
//  Original author: bfzhao
///////////////////////////////////////////////////////////
#pragma once


#include <vector>
#include <map>
#include <list>
#include <sstream>
#include <fstream>
#include <ostream>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include  <io.h>

using namespace std;

struct Quate{
    float x, y, z, w;

};
#ifdef API_EXPORTS
#define DLL_API __declspec(dllexport)  
#else
#define DLL_API 
#endif
/**
	* 文件操作类
	*/
class  DLL_API FileLibrary
{

public:
	FileLibrary();

	virtual ~FileLibrary();

    static FileLibrary * getInstance();

    bool isDirExists(const std::string& strArgFilePath);
	/**
		* 判断文件是否存在
		*/
	bool isFileExists(const std::string& strArgFilePath);
	/**
		* 从路径获取文件名称
		*/
	std::string getFileNameFromPath(const std::string& strFilePath);


	/**
		* 组合字符串路径
		*/
	std::string combineFilePath(const std::string prm1, const std::string prm2);

	std::string convertToWinPath(const std::string prm1);

	std::string convertToLinuxPath(const std::string prm1);

	/** 获取当前路径*/
	std::string getCurrentFilePath();

	/**
		* 获取所有子文件和目录
		*/
	void getAllSubFiles(const std::string& strArgFilePath, list<std::string>& lstSubFiles, bool bIncludeDirs = true, bool bIncludeFiles = true, bool bRecursive = true, const std::string& strFilter = std::string(""));

    /**获取系统当前时间**/
	std::string getCurrentTime();

    /**字符串分割函数**/
	vector< std::string> splitString(std::string str, std::string pattern, vector<std::string>& vectstring);


	bool readFile(const std::string& filepath, vector<std::string>& vecFileInfo, const std::string& type);

    /**获取文件的父路径**/
    std::string getFileParentPath(const std::string& strFilePath);

    /**复制文件**/
    int copyFile(const std::string &src,const std::string &dst);

    std::string getRand();

	/**RGB转HSV颜色值**/
    bool Rgb2Hsv(float R, float G, float B);

    /**字符串转数值函数**/
    template <class T> T convertFromstring(T &value, const std::string &s);
    template <class T> std::string convertTostring(T &value, int precision = 9);

    /**四元数转roll, pitch, yaw**/
    void toEulerAngle(const Quate& q, double& roll, double& pitch, double& yaw);
	
	/**过滤绿色rgb值函数**/
    bool filterGreen(float R, float G, float B, float green);

	template<class T> int	parse_arg(int argc, char** argv, const char* argument_name, T& value1)
	{
		int index = find_argument(argc, argv, argument_name) + 1;

		if (index > 0 && index < argc)
		{
			std::istringstream stream;
			stream.clear();
			stream.str(argv[index]);
			stream >> value1;
		}

		return (index - 2);
	}

	template<class T> int	parse_2x_arg(int argc, char** argv, const char* argument_name, T& value1, T& value2)
	{
		int index = find_argument(argc, argv, argument_name) + 1;

		if (index > 0 && index < argc)
		{
			std::istringstream stream;
			stream.clear();
			stream.str(argv[index]);
			stream >> value1;
		}

		index = find_argument(argc, argv, argument_name) + 2;

		if (index > 0 && index < argc)
		{
			std::istringstream stream;
			stream.clear();
			stream.str(argv[index]);
			stream >> value2;
		}

		return (index - 2);
	}


	template<class T> int parse_3x_arg(int argc, char** argv, const char* argument_name, T& value1, T& value2, T& value3)
	{
		int index = find_argument(argc, argv, argument_name) + 1;

		if (index > 0 && index < argc)
		{
			std::istringstream stream;
			stream.clear();
			stream.str(argv[index]);
			stream >> value1;
		}

		index = find_argument(argc, argv, argument_name) + 2;

		if (index > 0 && index < argc)
		{
			std::istringstream stream;
			stream.clear();
			stream.str(argv[index]);
			stream >> value2;
		}

		index = find_argument(argc, argv, argument_name) + 3;

		if (index > 0 && index < argc)
		{
			std::istringstream stream;
			stream.clear();
			stream.str(argv[index]);
			stream >> value3;
		}

		return (index - 3);
	}
private:
	static FileLibrary* m_pInstance;
	

};

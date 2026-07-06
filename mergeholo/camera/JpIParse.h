#pragma once
#ifndef DLL_API
#ifdef DLL_EXPORT
#if defined(_MSC_VER)
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __attribute__((visibility("default")))
#endif
#else
#if defined(_MSC_VER)
#define DLL_API __declspec(dllimport)
#else
#define DLL_API
#endif
#endif
#endif

#ifdef WIN64
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/statfs.h>  
#endif
#include <opencv2/core.hpp>

namespace jp_lightfield {
	struct strLightFieldInput {
		bool bmono;
		cv::Mat data;
		float tempreture[3];
		strLightFieldInput() : bmono(false){
			tempreture[0] = 0.0f;
			tempreture[1] = 0.0f;
			tempreture[2] = 0.0f;
		}
	};
	struct strLightFieldOutput {
		cv::Mat img2d;
		cv::Mat depthMap;
		cv::Mat img3d;
	};
	enum enParseType
	{
		parseSlow = 0,
		parseQuick
	};
	class DLL_API JpIParse {
	public:
		JpIParse() {};
		virtual ~JpIParse() {};

		/*************************/
		static JpIParse* GetIParse(enParseType type = parseSlow);
		static void ReleaseIParse(JpIParse* iparse);
	public:
		virtual int Init(std::string cfgPath, int gpuId) = 0;
		virtual int Parse(strLightFieldInput& lfdata, 
			strLightFieldOutput& result) = 0;
	};
}
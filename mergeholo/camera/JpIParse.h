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

#include <string>

#ifdef WIN64
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
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

namespace jp_lightfield {
	struct strLightFieldInput {
		int channel;
		int dataLen;
		unsigned char * data;
		float tempreture[3];
		strLightFieldInput() : channel(1), dataLen(1), data(0){
			tempreture[0] = 0.0f;
			tempreture[1] = 0.0f;
			tempreture[2] = 0.0f;
		}
	};
	struct strLightFieldOutput {
		unsigned char* img2d;
		unsigned char* depthMap;
		float* img3d;
		float* imgTest;

		unsigned char* img2dCuda;
		float* img3dCuda;

		strLightFieldOutput() :
			img2d(0),
			depthMap(0), img3d(0), imgTest(0),
			img2dCuda(0), img3dCuda(0) {};
	};
	class DLL_API JpIParse {
	public:
		JpIParse() {};
		virtual ~JpIParse() {};

		/*************************/
		static JpIParse* GetIParse();
		static void ReleaseIParse(JpIParse* iparse);
	public:
		virtual int Init(std::string cfgPath, int gpuId, 
			int& height, int& width) = 0;
		virtual int Parse(
			strLightFieldInput&  lfdata, 
			strLightFieldOutput& result) = 0;
		virtual bool ParamCorrection(
			int* pDataX,
			int* pDataY,
			int nData, 
			float realDis,
			const strLightFieldOutput& result) = 0;
	};
}

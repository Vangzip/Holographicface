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
#include <string>

namespace jp_lightfield {

#ifdef WIN64
    struct timeval
    {
        long tv_sec;
        long tv_usec;
    };
#endif

	struct strCameraConf {
		int exposeMode; //???????0?????, 1?????1, 2?????2
		int exposeVal;
		int id;         //in
		double frameRate;
		int missThresold;

		int width;      //out
		int height;     //out
		bool bColor;    //out//???/??????

		strCameraConf() : exposeMode(1) {
			exposeVal = 1000;
			id = 0;
			frameRate = 10.0;
			missThresold = 0;
		}
	};
	struct strCameraData {
		int dataLen;
		unsigned char* data;
		float tempreture;
		struct timeval tv;
		bool bmono;
		strCameraData() :dataLen(1), data(0), tempreture(0.0f), bmono(false) {
		}
	};
	class DLL_API JpICamera {
	public:
		JpICamera() {};
		virtual ~JpICamera() {};

		/*************************/
		//
		static JpICamera* GetICamera(std::string camSeri = "CXP");
		static void ReleaseICamera(JpICamera* icam);

	public:
		/*camType: ????????
		65M,
		GM21M12X4,
		*/
		virtual int Init(strCameraConf& conf, std::string camType) = 0;
		virtual int Capture(strCameraData& cd) = 0;
		virtual void Free(strCameraData& cd) = 0;
		virtual int SetExpose(int mode, int val) = 0;
	};
}

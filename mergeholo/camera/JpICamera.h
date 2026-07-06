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

    #ifdef WIN64
        struct timeval
        {
            long tv_sec;  // 秒
            long tv_usec; // 微秒
        };
    #endif
	struct strCameraConf {
		int exposeMode; //曝光模式，0：手动, 1：自动1, 2：自动2
		int exposeVal[3];
		int id;         //in
		double frameRate;
		int missThresold;

		int width;      //out
		int height;     //out
		bool bColor;    //out//彩色/黑白相机

		strCameraConf() : exposeMode(1) {
			exposeVal[1] = 1000;
			id = 0;
			frameRate = 10.0;
			missThresold = 0;
		}
	};
	struct strCameraData {
		cv::Mat data;
		float tempreture;
		struct timeval tv;
		bool bmono;
		strCameraData() : tempreture(0.0f), bmono(false) {
		}
	};
	class DLL_API JpICamera {
	public:
		JpICamera() {};
		virtual ~JpICamera() {};

		/*************************/
		//
		static JpICamera* GetICamera();
		static void ReleaseICamera(JpICamera* icam);

	public:
		virtual int Init(strCameraConf& conf) = 0;
		virtual int Capture(strCameraData& cd) = 0;
		virtual int SetExpose(int mode, int val) = 0;
	};
}

#ifndef DATASTRUCTURE_H
#define DATASTRUCTURE_H

#include <iostream>
#include <vector>
//#include <opencv2/core.hpp>

//#define camNums 4
//#define sviewCamNums 4
#define maxOwn(a,b) (((a) > (b)) ? (a) : (b))

typedef struct stDistortion
{
    double fx;
    double fy;
    double cx;
    double cy;
    double k1;
    double k2;
}stDistortion;

typedef struct structCamCfg
{
    std::string ip;
    stDistortion distortion;
    std::string videoFile;
    std::string imgName;
//    double fx;
//    double fy;
//    double cx;
//    double cy;
//    double k1;
//    double k2;
}structCamCfg;


struct H264Frame
{
    uint64_t frameId;
    int64_t timeStamp;
    int dataLen;
    uint8_t *data;
};

struct stHeadElement
{
    int len;
    uint64_t fid;
    stDistortion distort;
//    double fx;
//    double fy;
//    double cx;
//    double cy;
//    double k1;
//    double k2;

    int offx;
    int offy;
};

struct stH264Head
{
    int idRecognition;
    int countStHead;
    int64_t timestamp;
    stHeadElement arrHeadElements[20];
    //std::vector<stHeadElement> vctElem;

    // size_t size()
    // {
    //     return sizeof(int)*2 + sizeof(int64_t) + countStHead*sizeof(stHeadElement);
    // }
};


//audio send info
struct stAudioData
{
    uint64_t timeStamp;
    unsigned char* data;
    int lenth;
};



//struct stSviewFrame
//{
//    int frameId;
//    uint64_t timeStamp;
//    int dataLen;
//    cv::Mat data;
//};

#pragma pack(1)
typedef struct stSendGps
{
    uint64_t timestamp;
    float lat;
    char latDirection;
    float lon;
    char lonDirection;
    float alt;//海拔高度
    float heading;//航向
    float pitch;//俯仰(正负90 deg)
}stSendGps;
#pragma pack()

typedef struct strctGPS
{
    uint64_t utcTime;
    float lat;
    char latDirection;
    float lon;
    char lonDirection;
    int state;//GPS state: 0 = 定位不可用或无效; 1 = 单点定位; 2 = 差分定位; 3 = GPS PPS 模式; 4 = RTK INT; 5 = RTK Float; 7 = 手动输入模式; 8 = 模拟器模式;
    int sats;
    float hdop;
    float alt;//海拔高度
} strctGPS;

typedef enum solutionState
{
    SOL_COMPUTED = 0, // 已解出
    INSUFFICIENT_OBS, // 观测数据不足
    NO_CONVERGENCE, // 无法收敛，输出解无效
    COV_TRACE // 协方差矩阵的迹超过最大值（迹>1000 米）
}solutionState;

typedef struct strctHeading
{
    int enumSolutionState;//解的状态
    int enumPosType; //位置类型
    float length;//基线长
    float heading;//航向
    float pitch;//俯仰(正负90 deg)
    float reserved;//保留
    float hdgstddev;//航向标准偏差
    float ptchstddev;//俯仰标准偏差

}strctHeading;


#endif // DATASTRUCTURE_H

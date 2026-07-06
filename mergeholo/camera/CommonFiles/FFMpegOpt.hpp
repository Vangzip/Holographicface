////////////////////////////////////////////////////////////////////////////////
//
// File Name:	dbmgr.hpp
// Class Name:  CDbMgr
// Description:	用于管理唯一的静态数据库对象，
// Author:		罗志彬
// Date:		2019年4月22日
//
////////////////////////////////////////////////////////////////////////////////

#ifndef FFMEPG_HPP
#define FFMEPG_HPP
#include <QString>
#include <QUdpSocket>
#include <vector>
#include <iostream>
#include <QThread>
#include <QDebug>
#include "CommonMacro.h"
#include <opencv2/opencv.hpp>
#include <QUrl>
#include <QTimer>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavfilter/avfilter.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
}
#include "JpCTime.hpp"

#pragma pack(1)
#if 0
using FrameTail = union sss
{
    uchar arr[21];
    struct
    {
        uchar s1[7];
        int64_t iTime;
        uchar s2[6];
    } st;
    void print()
    {

        for(int i=0;i<21;i++)
        {
            printf("%02x ",arr[i]);
        }
        printf("\n");

    }
    void swap()
    {
        arr[7]^=arr[14];
        arr[14]^=arr[7];
        arr[7]^=arr[14];

        arr[8]^=arr[13];
        arr[13]^=arr[8];
        arr[8]^=arr[13];

        arr[9]^=arr[12];
        arr[12]^=arr[9];
        arr[9]^=arr[12];

        arr[10]^=arr[11];
        arr[11]^=arr[10];
        arr[10]^=arr[11];
    }
} ;
#else
using FrameTail = union sss
{
    uchar arr[8];
    struct
    {
        unsigned long long  iTime;
    } st;

    void swap()
    {
        arr[0]^=arr[7];
        arr[7]^=arr[0];
        arr[0]^=arr[7];

        arr[1]^=arr[6];
        arr[6]^=arr[1];
        arr[1]^=arr[6];

        arr[2]^=arr[5];
        arr[5]^=arr[2];
        arr[2]^=arr[5];

        arr[3]^=arr[4];
        arr[4]^=arr[3];
        arr[3]^=arr[4];
    }
};
#endif
#pragma pack()


std::map<AVFormatContext*,bool> gMapRunning;
int interrupt_cb(void *ctx)
{
    AVFormatContext* pCtx = static_cast<AVFormatContext*>(ctx);
    if( gMapRunning[pCtx] )
    {
        // printf("%x running\n",pCtx);
        return 0;
    }
    else
    {
        // printf("%x quit\n",pCtx);
        return 1;
    }
}

class CFFMpeg264Opt
{
public:
    using Ptr = std::shared_ptr<CFFMpeg264Opt>;
    using DataType = std::vector<unsigned char>;

    FILE* fp = nullptr;
public:

//    int interrupt_cb(void *ctx)
//    {

//        if( !ctx )
//        {
//            return 1;
//        }
//        return 0;
//    }

    void init()
    {
        //初始化FFMPEG  调用了这个才能正常适用编码器和解码器
        pFormatCtx = avformat_alloc_context();  //init FormatContext
        pFormatCtx->interrupt_callback.callback = interrupt_cb;
        pFormatCtx->interrupt_callback.opaque = pFormatCtx;
        gMapRunning[pFormatCtx] = true;
        //初始化FFmpeg网络模块
        avformat_network_init();    //init FFmpeg network


        //fp = fopen("test.265","wb+");
        m_pUdp = new QUdpSocket(nullptr);
        m_pUdp->setSocketOption(QAbstractSocket::MulticastTtlOption, 1);

    }

    bool open( std::string strUrl , int iProtocol =0)
    {
        //设置参数

        AVDictionary *format_opts = NULL;
        av_dict_set(&format_opts, "stimeout", std::to_string( 500 * 1000000).c_str(), 0); //设置链接超时时间（us）
        av_dict_set(&format_opts, "timeout",  std::to_string( 500 * 1000000).c_str(), 0); //设置链接超时时间（us）

        if( iProtocol == 0 )
        {
            av_dict_set(&format_opts, "rtsp_transport",  "tcp", 0); //设置推流的方式，默认udp
            // std::cout<<" Camera use tcp " << std::endl;

        }
        else
        {
            // av_dict_set(&format_opts, "rtsp_transport",  "udp", 0); //设置推流的方式，默认udp
            // av_dict_set_int(&format_opts, "localport", 91152, 0);
            std::cout<<" Camera use udp " << std::endl;

        }


        av_dict_set(&format_opts, "buffer_size", "2048000", 0);
        //av_dict_set(&format_opts, "fflags", "nobuffer", 0);
        av_dict_set(&format_opts, "reorder_queque_size", "1000", 0);
        av_dict_set(&format_opts, "max_delay", "10000000", 0);

        m_strUrl = strUrl;

        QUrl ip(strUrl.c_str());
        LOG_OUTPUT <<"Try to open camera ["<< ip.host() <<":"<<ip.port() << "] ...";

        ret = avformat_open_input(&pFormatCtx,strUrl.c_str(),nullptr,&format_opts);
        if(ret != 0){
            av_strerror(ret,errors,sizeof(errors));
            LOG_ERROR <<"Failed to open camera ["<< ip.host() <<":"<<ip.port() << "]: "<< errors;
            return false;
        }

        //Get audio information
        //std::cout  << "avformat_find_stream_info " <<std::endl;
        ret = avformat_find_stream_info(pFormatCtx,nullptr);
        if(ret != 0){
            av_strerror(ret,errors,sizeof(errors));
            LOG_ERROR<<"Failed to open stream ["<< ip.host() <<":"<<ip.port() << "]: "<< errors;
            return false; ;
//            /exit(ret);
        }


        //循环查找视频中包含的流信息，直到找到视频类型的流
        //便将其记录下来 videoIndex
        //这里我们现在只处理视频流  音频流先不管他
        for (i = 0; i < pFormatCtx->nb_streams; i++) {
            if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoIndex = i;
            }
        }

        //如果videoIndex为-1 说明没有找到视频流
        if (videoIndex == -1) {
            LOG_ERROR<<"Failed to open stream ["<< ip.host() <<":"<<ip.port() << "]: "<< errors;
            return false;;
        }


        //配置编码上下文，AVCodecContext内容
        //1.查找解码器
        pCodec = avcodec_find_decoder(
                    pFormatCtx->streams[videoIndex]->codecpar->codec_id);
        //2.初始化上下文
        pCodecCtx = avcodec_alloc_context3(pCodec);
        //pCodecCtx->thread_count = 2;
        //3.配置上下文相关参数
        avcodec_parameters_to_context(pCodecCtx,pFormatCtx->streams[videoIndex]->codecpar);
        //4.打开解码器
        ret = avcodec_open2(pCodecCtx, pCodec, nullptr);
        if(ret != 0){
            av_strerror(ret,errors,sizeof(errors));
            LOG_ERROR <<"Failed to open camera ["<< ip.host() <<":"<<ip.port()<< "]: "<< errors;
            // exit(ret);
            return false; ;
        }

        //初始化视频帧
        pFrame = av_frame_alloc();
        pFrameRGB = av_frame_alloc();
        //为out_buffer申请一段存储图像的内存空间
        out_buffer = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB24,pCodecCtx->width,pCodecCtx->height,1));
        //实现AVFrame中像素数据和Bitmap像素数据的关联
        av_image_fill_arrays(pFrameRGB->data,pFrameRGB->linesize, out_buffer,
                             AV_PIX_FMT_RGB24,pCodecCtx->width, pCodecCtx->height,
                             1);
        //为AVPacket申请内存
        packet = (AVPacket *)av_malloc(sizeof(AVPacket));
        //打印媒体信息
        // av_dump_format(pFormatCtx,0,strUrl.c_str(),0);
        //初始化一个SwsContext
        img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                         pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                                         AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);
//        if (m_pUdp->bind(QHostAddress("192.168.1.123"), 21022, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) //先绑定端口
//        {
//            bool bRs = m_pUdp->joinMulticastGroup(QHostAddress(m_strGroupIp)); //加入IP地址为groupAddress的多播组，绑定端口groupPort进行通信
//            if( !bRs )
//            {
//                std::cout<<"Join Error" << std::endl;
//            }
//        }

/*        m_pTimer.reset(new QTimer);
        m_pTimer->callOnTimeout(*/
        std::string strFile = QString("./calc/%1.%2.mp4").arg(ip.host()).arg(ip.port()).toStdString();
        //ip.host() <<":"<<ip.port()
        avformat_alloc_output_context2(&pFormatCtxOutput,NULL, NULL,strFile.c_str());

//        m_pHeartBit.reset(new std::thread(
//        [&]()
//        {


//            while(pFormatCtxOutput)
//            {
//                AVDictionary* options = NULL;
//                av_dict_set(&options, "rtsp_flags", "get_parameter_request", 0);
//                avformat_write_header(pFormatCtxOutput, &options);
//                std::cout<<"Get Param Request" << std::endl;
//                usleep(3000000);
//            }

//            //QThread::sleep(1);
//        }) );
//        //m_pTimer->start(3000);

        LOG_OUTPUT <<"Open camera ["<< ip.host() <<":"<<ip.port() << "] done!";
        return true;
        // qDebug() << "open url done!" ;
    }

    std::vector<unsigned char> getFrame()
    {

        // JpCtime o("getFrame");
        std::vector<unsigned char> vctData;
        if( !packet  || !pFormatCtx )
        {
            return vctData;
        }

        while( true )
        {
            int rs = av_read_frame(pFormatCtx,packet);
            if( rs < 0 )
            {
                av_strerror(rs,errors,sizeof(errors));
                return vctData;
            }
            if( packet->size <=0  )
            {
                QThread::msleep(1);
                av_strerror(rs,errors,sizeof(errors));
                return vctData;
            }

            // printf("size : %d\n",packet->size);

            //判断视频帧
            if(packet->stream_index == videoIndex)
            {
                vctData.resize(packet->size);
                memcpy(vctData.data(),packet->data, packet->size);
                if( fp )
                {
                    fwrite(const_cast<uint8_t*>(packet->data),1,packet->size -21,fp);
                    //fwrite(vctData.data(),sizeof(unsigned char), packet->size,fp);
                }
                av_packet_unref(packet);
                return vctData;
            }
            av_packet_unref(packet);
        }

        return vctData;
    }

    bool getDecodeFrame( cv::Mat& frame,std::vector<unsigned char>& vctData, FrameTail&d)
    {
        // JpCtime o("getDecodeFrame");
        if( !packet  || !pFormatCtx )
        {
            return false;
        }
        do
        {
            int rs = av_read_frame(pFormatCtx,packet);
            if( rs < 0 )
            {
                QThread::msleep(1);
                av_strerror(rs,errors,sizeof(errors));
                continue;
                //return false;
            }
            if( packet->size <=0  )
            {
                QThread::msleep(1);
                av_strerror(rs,errors,sizeof(errors));
                continue;
                //return false;
            }


            //printf("size : %d\n",packet->size);

            //判断视频帧
            if(packet->stream_index == videoIndex)
            {


                int ioffset = 0;
                if( packet->size <=21  )
                {
                    ioffset =0 ;
                }
                // printf("size : %d\n",packet->size);
                vctData.resize(packet->size);
                memcpy(vctData.data(),packet->data, packet->size);
                memcpy(d.arr, packet->data+packet->size-ioffset,ioffset);

                // printf("Get frame : %x %x %x %x\n", vctData[0],vctData[1],vctData[2],vctData[3]);
                if( fp )
                {
                    fwrite(vctData.data(),sizeof(unsigned char), packet->size,fp);
                }

                packet->size -= ioffset;
                auto ret = avcodec_send_packet(pCodecCtx, packet);
                if(ret != 0)
                {
                    av_strerror(ret,errors,sizeof(errors));
                    std::cout <<"Failed to send packet : ["<< ret << "]"<< errors<< std::endl;
                    return false;
                }

                //ret = avcodec_receive_frame(pCodecCtx, pFrame);
                int iCnt = 3;
                while( iCnt )
                {
                    ret = avcodec_receive_frame(pCodecCtx, pFrame);
                    if( ret != 0 )
                    {
                        av_strerror(ret,errors,sizeof(errors));
                        iCnt --;
                        if( iCnt == 0 )
                        {
                            return false;
                        }
                        QThread::msleep(20);
                    }
                    break;

                }
                if (ret == 0)
                {
                    if( frame .empty() )
                    {
                        frame = cv::Mat(pCodecCtx->height,pCodecCtx->width,CV_8UC3);
                    }

                    //处理图像数据
                    sws_scale(img_convert_ctx,
                              (const unsigned char* const*) pFrame->data,
                              pFrame->linesize, 0, pCodecCtx->height,
                              &(frame.data),
                              pFrameRGB->linesize);
                    av_packet_unref(packet);
                    QThread::msleep(1);
                    return true;
                    //释放前需要一个延时
                    //QThread::msleep(1);
                }
            }
            av_packet_unref(packet);
            return false;
        }while( true );
    }

    void fini()
    {
        gMapRunning[pFormatCtx] = false;
        QThread::msleep(50);

        if( pFormatCtx )
        {
            avformat_close_input(&pFormatCtx);
            pFormatCtx = nullptr;
            //avcodec_free_context(&pFormatCtx);
        }
        if( pFormatCtxOutput )
        {
            avformat_close_input(&pFormatCtx);
            avformat_free_context(pFormatCtx);
            pFormatCtxOutput = nullptr;
            //avcodec_free_context(&pFormatCtx);
        }



        if( out_buffer )
        {
            av_free(out_buffer);
            out_buffer = nullptr;
        }
        if( pFrameRGB )
        {
            av_free(pFrameRGB);
            pFrameRGB = nullptr;
        }
        if( img_convert_ctx )
        {
            sws_freeContext(img_convert_ctx);
            img_convert_ctx = nullptr;
        }
        if( pCodecCtx )
        {

            avcodec_close(pCodecCtx);
            avcodec_free_context(&pCodecCtx);
            pCodecCtx = nullptr;

        }

        if( m_pUdp )
        {
            m_pUdp->leaveMulticastGroup(QHostAddress(m_strGroupIp));// 退出组播
            m_pUdp->abort();
            //m_pUdp->disconnect();
            delete m_pUdp;
            m_pUdp = nullptr;
        }
//        if(m_pTimer.get() )
//        {
//            m_pTimer->stop();
//        }
        if(m_pHeartBit.get() )
        {
            m_pHeartBit->join();
        }
    }
public:

    AVFormatContext *pFormatCtx = nullptr;

    AVFormatContext *pFormatCtxOutput = nullptr;
    AVCodecContext *pCodecCtx = nullptr;
    const AVCodec *pCodec = nullptr;
    AVFrame *pFrame =nullptr;
    AVFrame *pFrameRGB = nullptr;
    AVPacket *packet = nullptr;
    struct SwsContext *img_convert_ctx = nullptr;

    unsigned char *out_buffer = nullptr;
    int i,videoIndex;
    int ret;
    char errors[1024] = "";

    //rtsp地址:rtsp://192.168.1.57/live/lrm0
    std::string m_strUrl = "rtsp://192.168.1.57/live/lrm0";
    QString m_strGroupIp = "224.0.0.123";
    int m_iPort = 21021;
    std::shared_ptr<std::thread> m_pHeartBit;
    //std::shared_ptr<QTimer> m_pTimer;
    QUdpSocket* m_pUdp = nullptr;
};

#endif // DBMGR_HPP

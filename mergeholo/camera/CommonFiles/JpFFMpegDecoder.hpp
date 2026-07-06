#include "JpDecoderInterface.h"
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace JP
{
    /// `JpFFMpegDecoder`类，继承自`IJpDecoderInterface`，用于视频解码
    class JpFFMpegDecoder : public IJpDecoderInterface
    {
    public:
        /// 解码上下文结构体，包含解码需要的各种上下文
        struct DecodeContext
        {
            AVCodecContext *codecCtx;     ///< 解码器上下文
            AVFormatContext *formatCtx;   ///< 格式上下文
            AVFrame *frame;               ///< 解码后的帧数据
            AVPacket *packet;             ///< 编码数据包
            struct SwsContext *swsCtx;    ///< 图像转换上下文
        };

        /// 创建解码流
        /**
         * @param stream 返回的解码流
         * @param nWidth 视频宽度
         * @param nHeight 视频高度
         * @param enumMethod 编码方法（H264, HEVC等）
         * @return 成功返回0，失败返回负数
         */
        int createStream(DecodeStream &stream, int nWidth, int nHeight, EncodeMethod enumMethod) override
        {
            av_register_all(); // 注册所有的格式和编解码器
            avcodec_register_all(); // 注册所有的编解码器

            AVCodec *codec = nullptr;
            // 根据编码方法选择解码器
            switch (enumMethod)
            {
            case H264:
                codec = avcodec_find_decoder(AV_CODEC_ID_H264); // 查找H264编码的解码器
                break;
            case HEVC:
                codec = avcodec_find_decoder(AV_CODEC_ID_HEVC); // 查找HEVC编码的解码器
                break;
            default:
                return -1; // 未知的编码方法
            }

            if (!codec)
            {
                return -1; // 找不到解码器
            }

            // 分配解码上下文
            DecodeContext *ctx = new DecodeContext();
            ctx->codecCtx = avcodec_alloc_context3(codec);
            if (!ctx->codecCtx)
            {
                delete ctx;
                return -1; // 无法分配解码上下文
            }

            // 设置解码器上下文的参数
            ctx->codecCtx->width = nWidth;
            ctx->codecCtx->height = nHeight;
            ctx->codecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P; // 设置适当的像素格式

            // 打开解码器
            if (avcodec_open2(ctx->codecCtx, codec, nullptr) < 0)
            {
                avcodec_free_context(&ctx->codecCtx);
                delete ctx;
                return -1; // 无法打开解码器
            }

            ctx->frame = av_frame_alloc(); // 分配帧
            ctx->packet = av_packet_alloc(); // 分配数据包
            ctx->swsCtx = sws_getContext(nWidth, nHeight, ctx->codecCtx->pix_fmt,
                                         nWidth, nHeight, AV_PIX_FMT_BGR24, SWS_BILINEAR, nullptr, nullptr, nullptr); // 设置图像转换上下文

            stream = reinterpret_cast<DecodeStream>(ctx); // 将上下文转换为解码流
            return 0;
        }

        /// 释放解码流
        /**
         * @param stream 要释放的解码流
         * @return 成功返回0
         */
        int releaseStream(DecodeStream &stream) override
        {
            DecodeContext *ctx = reinterpret_cast<DecodeContext *>(stream);
            if (ctx)
            {
                sws_freeContext(ctx->swsCtx); // 释放图像转换上下文
                av_frame_free(&ctx->frame); // 释放帧
                av_packet_free(&ctx->packet); // 释放数据包
                avcodec_free_context(&ctx->codecCtx); // 释放解码器上下文
                delete ctx; // 释放解码上下文
            }
            return 0;
        }

        /// 解码视频流
        /**
         * @param streamId 解码流ID
         * @param pBuffer 输入数据缓冲区
         * @param iBufferLen 输入数据长度
         * @param frame 输出的解码后的帧
         * @return 成功返回0，失败返回负数
         */
        int decode(const DecodeStream &streamId, const uint8_t *pBuffer, int iBufferLen, cv::Mat &frame) override
        {
            DecodeContext *ctx = reinterpret_cast<DecodeContext *>(streamId);
            if (!ctx)
            {
                return -1; // 无效的解码流
            }

            av_packet_unref(ctx->packet); // 重置数据包
            ctx->packet->data = const_cast<uint8_t *>(pBuffer); // 设置数据包的数据
            ctx->packet->size = iBufferLen; // 设置数据包的大小

            int ret = avcodec_send_packet(ctx->codecCtx, ctx->packet); // 发送数据包到解码器
            if (ret < 0)
            {
                return ret; // 发送数据包出错
            }

            ret = avcodec_receive_frame(ctx->codecCtx, ctx->frame); // 从解码器接收解码后的帧
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                return ret; // 需要更多数据或者已到流的末尾
            }
            else if (ret < 0)
            {
                return ret; // 解码出错
            }

            // 转换帧数据格式为OpenCV格式
            cv::Mat img(ctx->codecCtx->height, ctx->codecCtx->width, CV_8UC3);
            uint8_t *data[1] = {img.data};
            int linesize[1] = {static_cast<int>(img.step[0])};

            sws_scale(ctx->swsCtx, ctx->frame->data, ctx->frame->linesize, 0, ctx->codecCtx->height, data, linesize); // 图像转换

            frame = img; // 设置输出帧
            return 0;
        }

        /// 对图像进行去畸变处理
        /**
         * @param streamId 解码流ID
         * @param param 去畸变参数
         * @param frame 输入和输出的图像
         * @return 成功返回0
         */
        int distortion(const DecodeStream &streamId, stDistortionParam param, cv::Mat &frame) override
        {
            // 创建相机矩阵
            cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) << param.fx, 0, param.cx, 0, param.fy, param.cy, 0, 0, 1);
            // 创建畸变系数矩阵
            cv::Mat distCoeffs = (cv::Mat_<double>(1, 5) << param.k1, param.k2, 0, 0, 0);
            cv::Mat undistortedFrame;
            cv::undistort(frame, undistortedFrame, cameraMatrix, distCoeffs); // 去畸变处理
            frame = undistortedFrame; // 设置输出帧
            return 0;
        }

        /// 获取错误字符串
        /**
         * @param iErrorCode 错误码
         * @return 错误字符串描述
         */
        std::string errorString(int iErrorCode) override
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE]; 
            av_strerror(iErrorCode, errbuf, AV_ERROR_MAX_STRING_SIZE); // 获取错误描述字符串
            return std::string(errbuf); // 返回错误字符串
        }
    };
};

win32:{
FFMPEG_PATH=D:\SoftWare\ffmpeg
INCLUDEPATH+=$$FFMPEG_PATH\include
PRE_TARGETDEPS += \
    $$FFMPEG_PATH\lib\avcodec.lib \
    $$FFMPEG_PATH\lib\avdevice.lib \
    $$FFMPEG_PATH\lib\avfilter.lib \
    $$FFMPEG_PATH\lib\avformat.lib \
    $$FFMPEG_PATH\lib\swresample.lib \
    $$FFMPEG_PATH\lib\swscale.lib \
LIBS += -L$$FFMPEG_PATH\lib\ -lavfilter -lavformat -lavcodec -lavdevice -lavutil -lswresample -lswscale \
}else:unix{
    PRE_TARGETDEPS += /usr/local/lib/libavcodec.a \
            /usr/local/lib/libavdevice.a \
            /usr/local/lib/libavfilter.a \
            /usr/local/lib/libavformat.a \
            /usr/local/lib/libavutil.a \
            /usr/local/lib/libswresample.a \
            /usr/local/lib/libswscale.a \

    LIBS += -L/usr/local/lib/ -lavfilter -lavformat -lavcodec -lavdevice   -lavutil -lswresample -lswscale -lm -lz -lX11 -lvdpau
}

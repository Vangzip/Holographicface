DEFINES+=JP_USE_OPENCV

win32:{
OPENCV_ROOT = $$(OPENCV_ROOT)
isEmpty(OPENCV_ROOT):exists($$PWD/../../opencv450/opencv/build/include/opencv2/core.hpp): OPENCV_ROOT = $$PWD/../../opencv450/opencv/build
isEmpty(OPENCV_ROOT):exists(D:/ljc/opencv450/build/include/opencv2/core.hpp): OPENCV_ROOT = D:/ljc/opencv450/build

isEmpty(OPENCV_ROOT): error("OpenCV 4.5.0 not found. Set OPENCV_ROOT to the OpenCV build directory.")

OPENCV_INC += $$OPENCV_ROOT/include
OPENCV_LIB += $$OPENCV_ROOT/x64/vc15/lib
} else:unix {
OPENCV_INC += /usr/include/opencv4
OPENCV_LIB += /usr/lib/aarch64-linux-gnu
}

INCLUDEPATH += $$OPENCV_INC

win32:CONFIG(release, debug|release):    LIBS += -L$$OPENCV_LIB -lopencv_world450
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OPENCV_LIB -lopencv_world450d
else:unix: LIBS += -L$$OPENCV_LIB -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio -lopencv_video -lopencv_calib3d

else:win32-g++:CONFIG(debug, debug|release):            PRE_TARGETDEPS += $$OPENCV_LIB/libopencv_world450.a
else:win32:!win32-g++:CONFIG(release, debug|release):   PRE_TARGETDEPS += $$OPENCV_LIB/opencv_world450.lib

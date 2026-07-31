QT += core gui widgets

CONFIG += c++17 console
TARGET = mergeholo
TEMPLATE = app

DEFINES += NOMINMAX _CRT_SECURE_NO_WARNINGS _HAS_STD_BYTE=0
DEFINES += _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

SOURCES += \
    apps/mergeholo_main.cpp \
    camera/CaptureSession.cpp \
    camera/LightFieldCapture.cpp \
    printing/Imc60gApi.cpp \
    printing/Imc60gMotionController.cpp \
    printing/PrintHardwareProfile.cpp \
    printing/PrintHardwarePreflight.cpp \
    printing/PrintFrame.cpp \
    printing/Sv660nExposureController.cpp \
    printing/V2PrintTiming.cpp \
    printing/PrintConfig.cpp \
    printing/PrintImageSource.cpp \
    printing/PrintJobRunner.cpp \
    printing/PrintController.cpp \
    printing/SecondScreenSelection.cpp \
    printing/V2D3DFramePresenter.cpp \
    vendor/base/FileLibrary.cpp \
    vendor/base/Logger.cpp \
    vendor/multiview/memoryAtlasPageSink.cpp \
    vendor/multiview/memoryFrameSink.cpp \
    vendor/multiview/multiviewAtlasPlan.cpp \
    vendor/multiview/multiviewAtlasRenderer.cpp \
    vendor/multiview/multiviewBatchRenderer.cpp \
    vendor/multiview/multiviewGraphicsConfig.cpp \
    vendor/multiview/multiviewRenderPlan.cpp \
    vendor/multiview/modelMoveHandler.cpp \
    vendor/point_cloud/src/ConverPointCloud.cpp \
    vendor/point_cloud/src/depth_io.cpp \
    vendor/point_cloud/src/modifiedPclFunctions.cpp \
    vendor/point_cloud/src/OdmTexturing.cpp \
    vendor/point_cloud/src/poissonmesh.cpp \
    widgets/CaptureWindow.cpp \
    widgets/Print9030Dialog.cpp

HEADERS += \
    camera/CaptureSession.h \
    camera/FrameChangeDetector.hpp \
    camera/JpICamera.h \
    camera/JpIParse.h \
    camera/LightFieldCapture.h \
    camera/CommonFiles/JPDeviceInterface.h \
    camera/CommonFiles/threadsafe_queue.hpp \
    printing/IImc60gApi.h \
    printing/Imc60gApi.h \
    printing/Imc60gMotionController.h \
    printing/IExposureController.h \
    printing/Sv660nExposureController.h \
    printing/V2PrintTiming.h \
    printing/IPrintFramePresenter.h \
    printing/IMotionController.h \
    printing/IVBlankWaiter.h \
    printing/PrintConfig.h \
    printing/PrintHardwarePreflight.h \
    printing/PrintFrame.h \
    printing/PrintImageSource.h \
    printing/PrintJobRunner.h \
    printing/PrintController.h \
    printing/SecondScreenSelection.h \
    printing/V2D3DFramePresenter.h \
    vendor/base/base.h \
    vendor/base/FileLibrary.h \
    vendor/base/Logger.hpp \
    vendor/multiview/ModelMoveCameraConfig.h \
    vendor/multiview/memoryAtlasPageSink.h \
    vendor/multiview/memoryFrameSink.h \
    vendor/multiview/multiviewAtlasPlan.h \
    vendor/multiview/multiviewAtlasRenderer.h \
    vendor/multiview/multiviewBatchRenderer.h \
    vendor/multiview/multiviewGraphicsConfig.h \
    vendor/multiview/multiviewRenderPlan.h \
    vendor/multiview/modelMoveHandler.h \
    vendor/point_cloud/include/ConverPointCloud.h \
    vendor/point_cloud/include/depth_io.h \
    vendor/point_cloud/include/modifiedPclFunctions.hpp \
    vendor/point_cloud/include/OdmTexturing.hpp \
    vendor/point_cloud/include/poissonmesh.hpp \
    widgets/CaptureWindow.h \
    widgets/Print9030Dialog.h

FORMS += \
    ui/CaptureWindow.ui \
    ui/Print9030Dialog.ui

INCLUDEPATH += \
    apps \
    camera \
    printing \
    widgets \
    vendor/base \
    vendor/multiview \
    vendor/point_cloud/include

include($$PWD/Pri/common.pri)
include($$PWD/Pri/imc60g.pri)
include($$PWD/Pri/opencv.pri)
include($$PWD/Pri/cuda.pri)
include($$PWD/Pri/holo_pipeline.pri)
include($$PWD/pipeline/PipelineModule.pri)

JP_LF_V4_ROOT = $$(JP_LF_V4_ROOT)
isEmpty(JP_LF_V4_ROOT):exists($$PWD/../holocamera/HoloTest/Holo_v4.1.1/windows/JpLFDll-v4.1.1.lib): JP_LF_V4_ROOT = $$PWD/../holocamera/HoloTest/Holo_v4.1.1
isEmpty(JP_LF_V4_ROOT): error("JpLFDll-v4.1.1 SDK not found. Set JP_LF_V4_ROOT or keep ../holocamera/HoloTest/Holo_v4.1.1 available.")

INCLUDEPATH += $$JP_LF_V4_ROOT/include
LIBS += -L$$JP_LF_V4_ROOT/windows -lJpLFDll-v4.1.1
win32:LIBS += -ld3d11 -ldxgi -ld3dcompiler -luser32

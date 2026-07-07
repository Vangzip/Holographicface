QT += core gui widgets

CONFIG += c++17 console
TARGET = mergeholo
TEMPLATE = app

DEFINES += NOMINMAX _CRT_SECURE_NO_WARNINGS _HAS_STD_BYTE=0
DEFINES += _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

SOURCES += \
    apps/CaptureWindow.cpp \
    apps/mergeholo_main.cpp \
    camera/CaptureSession.cpp \
    camera/LightFieldCapture.cpp \
    pipeline/CaptureImport.cpp \
    pipeline/ElementalStage.cpp \
    pipeline/HoloPipeline.cpp \
    pipeline/PipelineConfig.cpp \
    pipeline/PipelineLogger.cpp \
    pipeline/PipelineTiming.cpp \
    vendor/base/FileLibrary.cpp \
    vendor/base/Logger.cpp \
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
    vendor/point_cloud/src/poissonmesh.cpp

HEADERS += \
    apps/CaptureWindow.h \
    camera/CaptureSession.h \
    camera/FrameChangeDetector.hpp \
    camera/JpICamera.h \
    camera/JpIParse.h \
    camera/LightFieldCapture.h \
    camera/CommonFiles/JPDeviceInterface.h \
    camera/CommonFiles/threadsafe_queue.hpp \
    pipeline/CaptureImport.h \
    pipeline/ElementalStage.h \
    pipeline/HoloPipeline.h \
    pipeline/PipelineConfig.h \
    pipeline/PipelineData.h \
    pipeline/PipelineLogger.h \
    pipeline/PipelineTiming.h \
    vendor/base/base.h \
    vendor/base/FileLibrary.h \
    vendor/base/Logger.hpp \
    vendor/multiview/ModelMoveCameraConfig.h \
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
    vendor/point_cloud/include/poissonmesh.hpp

INCLUDEPATH += \
    apps \
    camera \
    pipeline \
    vendor/base \
    vendor/multiview \
    vendor/point_cloud/include

include($$PWD/Pri/common.pri)
include($$PWD/Pri/opencv.pri)
include($$PWD/Pri/cuda.pri)
include($$PWD/Pri/holo_pipeline.pri)

HOLO_SDK_ROOT = $$(HOLO_SDK_ROOT)
isEmpty(HOLO_SDK_ROOT):exists($$PWD/runtime/holoLib/JpLF-v3.1.lib): HOLO_SDK_ROOT = $$PWD/runtime/holoLib
isEmpty(HOLO_SDK_ROOT):exists($$PWD/../holocamera/HoloTest/holoLib/JpLF-v3.1.lib): HOLO_SDK_ROOT = $$PWD/../holocamera/HoloTest/holoLib
isEmpty(HOLO_SDK_ROOT): error("JpLF-v3.1 SDK not found. Set HOLO_SDK_ROOT or keep ../holocamera/HoloTest/holoLib available.")

INCLUDEPATH += $$HOLO_SDK_ROOT
LIBS += -L$$HOLO_SDK_ROOT -lJpLF-v3.1

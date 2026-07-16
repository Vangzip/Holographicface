QT -= gui
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = result_persistence_tests

DEFINES += NOMINMAX _CRT_SECURE_NO_WARNINGS _HAS_STD_BYTE=0
win32:QMAKE_CXXFLAGS += /source-charset:utf-8 /execution-charset:utf-8

INCLUDEPATH += \
    .. \
    ../elemental \
    ../multiview \
    ../stages \
    ../../vendor/base \
    ../../vendor/multiview

SOURCES += \
    test_result_persistence.cpp \
    ../elemental/ElementalMemoryTransform.cpp \
    ../elemental/ElementalProcessor.cpp \
    ../PipelineConfig.cpp \
    ../PipelineTiming.cpp \
    ../ResultPersistence.cpp \
    ../../vendor/multiview/memoryAtlasPageSink.cpp \
    ../../vendor/multiview/memoryFrameSink.cpp \
    ../../vendor/multiview/multiviewAtlasPlan.cpp \
    ../../vendor/multiview/multiviewRenderPlan.cpp

HEADERS += \
    ../PipelineConfig.h \
    ../PipelineContext.h \
    ../ResultPersistence.h \
    ../ResultSaveSettings.h

include(../../Pri/opencv.pri)
include(../../Pri/holo_pipeline.pri)

LIBS += -lpcl_io -lpcl_io_ply -lpcl_common

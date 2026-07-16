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
    ../PipelineConfig.cpp

HEADERS += \
    ../PipelineConfig.h \
    ../PipelineContext.h

include(../../Pri/opencv.pri)
include(../../Pri/holo_pipeline.pri)

QT += core

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = processing_settings_tests

win32:QMAKE_CXXFLAGS += /source-charset:utf-8 /execution-charset:utf-8

INCLUDEPATH += \
    ../../settings \
    ../../pipeline \
    ../../vendor/multiview

SOURCES += \
    test_processing_settings.cpp \
    ../../settings/ProcessingSettings.cpp

HEADERS += \
    ../../settings/ProcessingSettings.h \
    ../../pipeline/PipelineInput.h \
    ../../pipeline/ResultSaveSettings.h \
    ../../vendor/multiview/ModelMoveCameraConfig.h

QT += core gui widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = processing_settings_tests

win32:QMAKE_CXXFLAGS += /source-charset:utf-8 /execution-charset:utf-8

INCLUDEPATH += \
    .. \
    ../../camera \
    ../../camera/CommonFiles \
    ../../settings \
    ../../pipeline \
    ../../vendor/multiview

SOURCES += \
    test_processing_settings.cpp \
    ../../settings/ProcessingSettings.cpp \
    ../../settings/KeyValueConfig.cpp \
    ../../settings/ProcessingSettingsStore.cpp \
    ../../camera/CaptureOrientation.cpp \
    ../ProcessingSettingsDialog.cpp \
    ../../pipeline/PipelineInput.cpp

HEADERS += \
    ../../settings/ProcessingSettings.h \
    ../../settings/KeyValueConfig.h \
    ../../settings/ProcessingSettingsStore.h \
    ../../camera/CaptureOrientation.h \
    ../ProcessingSettingsDialog.h \
    ../../pipeline/PipelineInput.h \
    ../../pipeline/DepthMeshModelMemory.h \
    ../../pipeline/ResultSaveSettings.h \
    ../../vendor/multiview/ModelMoveCameraConfig.h

FORMS += \
    ../../ui/ProcessingSettingsDialog.ui

include(../../Pri/opencv.pri)
include(../../Pri/holo_pipeline.pri)

LIBS += -lpcl_io -lpcl_io_ply -lpcl_common

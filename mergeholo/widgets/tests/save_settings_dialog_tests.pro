QT += core gui widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = save_settings_dialog_tests

win32:QMAKE_CXXFLAGS += /source-charset:utf-8 /execution-charset:utf-8

INCLUDEPATH += \
    .. \
    ../../pipeline \
    ../../vendor/multiview

SOURCES += \
    test_save_settings_dialog.cpp \
    ../InputSettingsDialog.cpp \
    ../NativeUiStyle.cpp \
    ../SaveSettingsDialog.cpp \
    ../../pipeline/PipelineInput.cpp

HEADERS += \
    ../NativeUiStyle.h \
    ../InputSettingsDialog.h \
    ../SaveSettingsDialog.h \
    ../../pipeline/DepthMeshModelMemory.h \
    ../../pipeline/PipelineInput.h \
    ../../pipeline/ResultSaveSettings.h

FORMS += \
    ../../ui/InputSettingsDialog.ui \
    ../../ui/SaveSettingsDialog.ui

include(../../Pri/opencv.pri)
include(../../Pri/holo_pipeline.pri)

LIBS += -lpcl_io -lpcl_io_ply -lpcl_common

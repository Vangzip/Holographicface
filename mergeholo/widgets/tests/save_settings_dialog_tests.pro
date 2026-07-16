QT += core gui widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = save_settings_dialog_tests

win32:QMAKE_CXXFLAGS += /source-charset:utf-8 /execution-charset:utf-8

INCLUDEPATH += \
    .. \
    ../../pipeline

SOURCES += \
    test_save_settings_dialog.cpp \
    ../SaveSettingsDialog.cpp

HEADERS += \
    ../SaveSettingsDialog.h \
    ../../pipeline/ResultSaveSettings.h

FORMS += \
    ../../ui/SaveSettingsDialog.ui

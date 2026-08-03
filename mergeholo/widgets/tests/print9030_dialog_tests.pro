QT += core gui widgets concurrent

CONFIG += console c++17
CONFIG -= app_bundle
DEFINES += PRINT9030_CONTROLLER_INTERFACE_ONLY
TEMPLATE = app
TARGET = print9030_dialog_tests

win32:QMAKE_CXXFLAGS += /utf-8

INCLUDEPATH += \
    .. \
    ../../printing \
    ../../pipeline \
    ../..

SOURCES += \
    test_print9030_dialog.cpp \
    ../Print9030Dialog.cpp \
    ../../printing/PrintConfig.cpp \
    ../../printing/PrintFrame.cpp \
    ../../printing/PrintImageSource.cpp

HEADERS += \
    ../Print9030Dialog.h \
    ../../printing/PrintController.h \
    ../../printing/PrintConfig.h \
    ../../printing/PrintImageSource.h

FORMS += ../../ui/Print9030Dialog.ui

include(../../Pri/opencv.pri)

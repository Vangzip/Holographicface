QT += core

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = capture_orientation_tests

win32:QMAKE_CXXFLAGS += /source-charset:utf-8 /execution-charset:utf-8

INCLUDEPATH += .. ../../vendor/multiview

SOURCES += \
    test_capture_orientation.cpp \
    ../CaptureOrientation.cpp

HEADERS += \
    ../CaptureOrientation.h \
    ../../vendor/multiview/multiviewCameraOrbit.h

include(../../Pri/opencv.pri)

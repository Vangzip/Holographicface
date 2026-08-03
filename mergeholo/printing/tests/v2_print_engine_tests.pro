QT += core gui
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = v2_print_engine_tests

INCLUDEPATH += .. ../../pipeline

SOURCES += \
    test_v2_print_engine.cpp \
    ../PrintFrame.cpp \
    ../PrintConfig.cpp \
    ../PrintPositionSampler.cpp \
    ../PrintHardwareProfile.cpp \
    ../PrintImageSource.cpp \
    ../PrintHardwarePreflight.cpp \
    ../PrintJobRunner.cpp \
    ../V2PrintTiming.cpp

HEADERS += \
    ../IExposureController.h \
    ../IMotionController.h \
    ../IPrintFramePresenter.h \
    ../PrintConfig.h \
    ../PrintPositionSampler.h \
    ../PrintFrame.h \
    ../PrintHardwarePreflight.h \
    ../PrintHardwareProfile.h \
    ../PrintImageSource.h \
    ../PrintJobRunner.h \
    ../V2PrintTiming.h

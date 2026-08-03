QT += core gui widgets
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = printing_tests

INCLUDEPATH += \
    ../.. \
    ../../pipeline \
    ../../pipeline/elemental \
    ..

SOURCES += \
    test_printing_modules.cpp \
    ../DfjzhMotionController.cpp \
    ../LegacyD3DImageRenderer.cpp \
    ../LegacyPrintTiming.cpp \
    ../LegacySecondScreenPresenter.cpp \
    ../PrintConfig.cpp \
    ../PrintHardwareProfile.cpp \
    ../PrintImageSource.cpp \
    ../PrintJobRunner.cpp \
    ../SecondScreenSelection.cpp

HEADERS += \
    ../PrintConfig.h \
    ../PrintHardwareProfile.h \
    ../DfjzhMotionController.h \
    ../IPrintFramePresenter.h \
    ../IMotionController.h \
    ../LegacyD3DImageRenderer.h \
    ../LegacyPrintTiming.h \
    ../LegacySecondScreenPresenter.h \
    ../PrintFrame.h \
    ../PrintImageSource.h \
    ../PrintJobRunner.h \
    ../SecondScreenSelection.h \
    ../../pipeline/elemental/ElementalMemoryResult.h

win32:LIBS += -ld3d11 -ldxgi -ld3dcompiler -luser32

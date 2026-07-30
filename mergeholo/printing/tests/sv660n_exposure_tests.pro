QT += core

CONFIG += c++17 console release
CONFIG -= app_bundle
TEMPLATE = app
TARGET = sv660n_exposure_tests

SOURCES += \
    test_sv660n_exposure.cpp \
    ../Sv660nExposureController.cpp \
    ../PrintHardwareProfile.cpp

HEADERS += \
    ../IImc60gApi.h \
    ../Sv660nExposureController.h \
    ../PrintHardwareProfile.h

INCLUDEPATH += ..

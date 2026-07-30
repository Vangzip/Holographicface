QT += core

CONFIG += c++17 console release
CONFIG -= app_bundle
TEMPLATE = app
TARGET = imc60g_motion_tests

SOURCES += \
    test_imc60g_motion.cpp \
    ../Imc60gMotionController.cpp

HEADERS += \
    ../IImc60gApi.h \
    ../Imc60gMotionController.h \
    ../PrintConfig.h \
    ../PrintHardwareProfile.h

INCLUDEPATH += \
    .. \
    ../../vendor/imc60g/include

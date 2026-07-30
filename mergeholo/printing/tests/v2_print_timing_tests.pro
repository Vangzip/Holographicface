QT = core
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = v2_print_timing_tests

INCLUDEPATH += ..

SOURCES += \
    test_v2_print_timing.cpp \
    ../V2PrintTiming.cpp \
    ../PrintConfig.cpp \
    ../PrintHardwareProfile.cpp

HEADERS += \
    ../V2PrintTiming.h \
    ../PrintConfig.h \
    ../PrintHardwareProfile.h

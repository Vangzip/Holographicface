QT += core
QT -= gui widgets
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = mesh_reconstruction_tests

DEFINES += NOMINMAX _CRT_SECURE_NO_WARNINGS _HAS_STD_BYTE=0
win32:QMAKE_CXXFLAGS += /source-charset:utf-8 /execution-charset:utf-8

INCLUDEPATH += \
    ../../base \
    ../include

SOURCES += \
    test_mesh_reconstruction.cpp \
    ../../base/FileLibrary.cpp \
    ../../base/Logger.cpp \
    ../src/ConverPointCloud.cpp \
    ../src/modifiedPclFunctions.cpp \
    ../src/OdmTexturing.cpp \
    ../src/poissonmesh.cpp

HEADERS += \
    ../include/ConverPointCloud.h \
    ../include/poissonmesh.hpp

include(../../../Pri/opencv.pri)
include(../../../Pri/holo_pipeline.pri)

win32:LIBS += -limagehlp

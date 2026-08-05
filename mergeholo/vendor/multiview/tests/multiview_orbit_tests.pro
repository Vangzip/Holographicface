QT += core
QT -= gui widgets
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = multiview_orbit_tests

INCLUDEPATH += ../../base ..
SOURCES += test_multiview_orbit.cpp ../multiviewRenderPlan.cpp
HEADERS += ../multiviewCameraOrbit.h ../multiviewOrbitMatrices.h ../multiviewRenderPlan.h

include(../../../Pri/holo_pipeline.pri)

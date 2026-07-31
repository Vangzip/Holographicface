QT += core gui widgets
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = v2_presenter_contract_tests
DEFINES += NOMINMAX

INCLUDEPATH += ..

SOURCES += \
    test_v2_presenter_contract.cpp \
    ../V2D3DFramePresenter.cpp \
    ../SecondScreenSelection.cpp

HEADERS += \
    ../IPrintFramePresenter.h \
    ../IVBlankWaiter.h \
    ../PrintFrame.h \
    ../SecondScreenSelection.h \
    ../V2D3DFramePresenter.h

win32:LIBS += -ld3d11 -ldxgi -ld3dcompiler -luser32

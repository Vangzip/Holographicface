QT = core
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = print_config_tests

INCLUDEPATH += ..

PROFILE_PATH = $$clean_path($$_PRO_FILE_PWD_/../../config/imc60g_print.ini)
DEFINES += IMC60G_PROFILE_PATH=\\\"$$PROFILE_PATH\\\"

SOURCES += \
    test_print_config.cpp \
    ../PrintConfig.cpp \
    ../PrintHardwareProfile.cpp

HEADERS += \
    ../PrintConfig.h \
    ../PrintHardwareProfile.h

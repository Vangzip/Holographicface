DESTDIR=$$PWD/../00-bin/

CONFIG(debug, debug|release) {
    MERGEHOLO_BUILD_VARIANT = debug
} else {
    MERGEHOLO_BUILD_VARIANT = release
}

UI_DIR =        $$PWD/../FF-tmp/ui/$$TARGET/$$MERGEHOLO_BUILD_VARIANT
MOC_DIR =       $$PWD/../FF-tmp/moc/$$TARGET/$$MERGEHOLO_BUILD_VARIANT
OBJECTS_DIR =   $$PWD/../FF-tmp/obj/$$TARGET/$$MERGEHOLO_BUILD_VARIANT
RCC_DIR =       $$PWD/../FF-tmp/rcc/$$TARGET/$$MERGEHOLO_BUILD_VARIANT

win32:QMAKE_CXXFLAGS += /source-charset:utf-8 /execution-charset:utf-8



DEFINES += LOG_OUT

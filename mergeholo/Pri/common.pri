DESTDIR=$$PWD/../00-bin/

UI_DIR =        $$PWD/../FF-tmp/ui/$$TARGET
MOC_DIR =       $$PWD/../FF-tmp/moc/$$TARGET
OBJECTS_DIR =   $$PWD/../FF-tmp/obj/$$TARGET
RCC_DIR =       $$PWD/../FF-tmp/rcc/$$TARGET

win32:QMAKE_CXXFLAGS += /source-charset:utf-8 /execution-charset:utf-8



DEFINES += LOG_OUT

IMC60G_ROOT = $$clean_path($$PWD/../vendor/imc60g)
IMC60G_INCLUDE_DIR = $$IMC60G_ROOT/include
IMC60G_LIB_DIR = $$IMC60G_ROOT/lib/x64
IMC60G_RUNTIME_DLL = $$IMC60G_ROOT/bin/x64/IMC_Library_x64.dll

!exists($$IMC60G_INCLUDE_DIR/IMC_Library.h): error("Missing IMC60G header")
!exists($$IMC60G_LIB_DIR/IMC_Library_x64.lib): error("Missing IMC60G x64 import library")
!exists($$IMC60G_RUNTIME_DLL): error("Missing IMC60G x64 runtime DLL")

INCLUDEPATH += $$IMC60G_INCLUDE_DIR
DEPENDPATH += $$IMC60G_INCLUDE_DIR
win32-msvc*:LIBS += /LIBPATH:$$shell_path($$IMC60G_LIB_DIR) IMC_Library_x64.lib

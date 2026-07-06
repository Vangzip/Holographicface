MERGEHOLO_ROOT = $$clean_path($$PWD/..)
THIRD_PARTY_ROOT = $$clean_path($$MERGEHOLO_ROOT/..)

DEFINES += BOOST_LIB_TOOLSET=\\\"vc142\\\"

win32-msvc*:QMAKE_CXXFLAGS += /FS /utf-8 /arch:AVX

PCL_ROOT = "$$THIRD_PARTY_ROOT/PCL 1.12.1-rc1"
OSG_ROOT = "$$THIRD_PARTY_ROOT/OSG365"
OE_ROOT = "$$THIRD_PARTY_ROOT/OE32"

INCLUDEPATH += \
    "$$PCL_ROOT/include/pcl-1.12" \
    "$$PCL_ROOT/3rdParty/Eigen/eigen3" \
    "$$PCL_ROOT/3rdParty/Boost/include/boost-1_78" \
    "$$PCL_ROOT/3rdParty/FLANN/include" \
    "$$PCL_ROOT/3rdParty/OpenNI2/Include" \
    "$$PCL_ROOT/3rdParty/VTK/include/vtk-9.1" \
    "$$PCL_ROOT/3rdParty/Qhull/include" \
    "$$OSG_ROOT/include" \
    "$$OE_ROOT/include"

LIBS += \
    -L"$$PCL_ROOT/lib" \
    -L"$$PCL_ROOT/3rdParty/Boost/lib" \
    -L"$$PCL_ROOT/3rdParty/FLANN/lib" \
    -L"$$PCL_ROOT/3rdParty/OpenNI2/Lib" \
    -L"$$PCL_ROOT/3rdParty/VTK/lib" \
    -L"$$PCL_ROOT/3rdParty/Qhull/lib" \
    -L"$$OSG_ROOT/lib" \
    -L"$$OE_ROOT/lib"

LIBS += \
    -lOpenThreads \
    -losg \
    -losgDB \
    -losgGA \
    -losgSim \
    -losgViewer \
    -losgEarth \
    -losgUtil \
    -losgTerrain \
    -losgFX \
    -losgText \
    -losgShadow \
    -lopengl32 \
    -losgParticle

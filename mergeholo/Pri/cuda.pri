

win32:{
CUDA_DIR = $$(CUDA_PATH)
isEmpty(CUDA_DIR): CUDA_DIR = "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.6"
# CUDA_SDK = "/usr/local/cuda"   # Path to cuda SDK install
} else:unix {
CUDA_DIR = "/usr/local/cuda-11.4"             # Path to cuda toolkit install
}

!exists($$CUDA_DIR/include/cuda.h) {
    warning("CUDA toolkit not found at $$CUDA_DIR; building without CUDA link libraries.")
    CONFIG += no_cuda
}




SYSTEM_NAME = linux         # Depending on your system either 'Win32', 'x64', or 'Win64'
SYSTEM_TYPE = 64            # '32' or '64', depending on your system
CUDA_ARCH = sm_72           # Type of CUDA architecture, for example 'compute_10', 'compute_11', 'sm_10'
NVCC_OPTIONS = --use_fast_math


win32:{
    !contains(CONFIG, no_cuda) {
        INCLUDEPATH  += $$CUDA_DIR/include
        #QMAKE_LIBDIR += $$CUDA_DIR/lib64/
        QMAKE_LIBDIR += $$CUDA_DIR/lib/x64/
    }
} else:unix {
    !contains(CONFIG, no_cuda) {
        INCLUDEPATH  += $$CUDA_DIR/include
        QMAKE_LIBDIR += $$CUDA_DIR/lib64/
    }
}


win32:!contains(CONFIG, no_cuda):CONFIG(release, debug|release):    LIBS +=  -L$$CUDA_DIR/lib/x64 -lcudart -lcufft -lcublas -lcurand -lcuda
else:win32:!contains(CONFIG, no_cuda):CONFIG(debug, debug|release): LIBS +=  -L$$CUDA_DIR/lib/x64 -lcudart -lcufft -lcublas -lcurand -lcuda
else:unix:!contains(CONFIG, no_cuda): LIBS += -L$$CUDA_DIR/lib64/ -lcudart -lcublasLt -lcublas -lcufft -lcufftw -lcublas  -lcuda
#-lcuda -lcudart -lcublasLt -lcublas -lcufft -lcufftw -lcublas -lcurand -lX11 -L/usr/local/lib-lX11 -lXrandr

# -lglfw3 -lGLEW -lGLU -lGL

CUDA_OBJECTS_DIR = ./

CUDA_LIBS = cudart cufft
CUDA_INC = $$join(INCLUDEPATH,'" -I"','-I"','"')
NVCC_LIBS = $$join(CUDA_LIBS,' -l','-l', '')



win32:!contains(CONFIG, no_cuda):CONFIG(debug, debug|release) {
    # Debug mode
    cuda_d.input = CUDA_SOURCES
    cuda_d.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o
    cuda_d.commands = $$CUDA_DIR/bin/nvcc -D_DEBUG $$NVCC_OPTIONS $$CUDA_INC $$NVCC_LIBS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH -c --compiler-options "/MDd" -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda_d.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda_d
} else:win32:!contains(CONFIG, no_cuda):CONFIG(release, debug|release): {
    # Release mode
    cuda.input = CUDA_SOURCES
    cuda.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o
    cuda.commands = $$CUDA_DIR/bin/nvcc $$NVCC_OPTIONS $$CUDA_INC $$NVCC_LIBS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH -O3 -c --compiler-options "/MD"  -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda
} else:unix:!contains(CONFIG, no_cuda) {
        cuda.input = CUDA_SOURCES
    cuda.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o
    cuda.commands = $$CUDA_DIR/bin/nvcc $$NVCC_OPTIONS $$CUDA_INC $$NVCC_LIBS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH -O3 -c --compiler-options "-fPIC" -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda
}

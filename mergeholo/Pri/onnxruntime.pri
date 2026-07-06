# onnxruntime.pri - 配置 ONNX Runtime (GPU, Windows, x64, v1.18.0)

ONNXRUNTIME_ROOT = D:/ljc/onnxruntime-win-x64-gpu-1.18.0

INCLUDEPATH += $$ONNXRUNTIME_ROOT/include

# 链接库路径
LIBS += -L$$ONNXRUNTIME_ROOT/lib

# 链接主库（GPU 版使用 onnxruntime.lib）
LIBS += -lonnxruntime  -lonnxruntime_providers_shared  -lonnxruntime_providers_cuda

# 注意：GPU 版本依赖 CUDA 和 cuDNN 的 DLL，但这些通常通过 PATH 加载，
# 不需要显式链接 .lib（除非你直接调用 CUDA API）

# 确保是 MSVC 编译器（ONNX Runtime GPU 版仅支持 MSVC，不支持 MinGW）
!contains(QMAKE_HOST.arch, x86_64): error("ONNX Runtime GPU requires x64")
!msvc: warning("ONNX Runtime GPU is built with MSVC; MinGW may not work!")

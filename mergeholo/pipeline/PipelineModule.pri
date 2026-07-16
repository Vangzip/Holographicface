PIPELINE_ROOT = $$PWD

SOURCES += \
    $$PIPELINE_ROOT/CaptureImport.cpp \
    $$PIPELINE_ROOT/elemental/ElementalAtlasDirectSink.cpp \
    $$PIPELINE_ROOT/elemental/ElementalMemoryTransform.cpp \
    $$PIPELINE_ROOT/elemental/ElementalProcessor.cpp \
    $$PIPELINE_ROOT/HoloPipeline.cpp \
    $$PIPELINE_ROOT/multiview/PclMeshOsgBuilder.cpp \
    $$PIPELINE_ROOT/PipelineConfig.cpp \
    $$PIPELINE_ROOT/PipelineLogger.cpp \
    $$PIPELINE_ROOT/PipelineTiming.cpp \
    $$PIPELINE_ROOT/ResultPersistence.cpp \
    $$PIPELINE_ROOT/stages/DepthStage.cpp \
    $$PIPELINE_ROOT/stages/ElementalStage.cpp \
    $$PIPELINE_ROOT/stages/MeshStage.cpp \
    $$PIPELINE_ROOT/stages/ModelStage.cpp \
    $$PIPELINE_ROOT/stages/MultiviewStage.cpp

HEADERS += \
    $$PIPELINE_ROOT/CaptureImport.h \
    $$PIPELINE_ROOT/DepthMeshModelMemory.h \
    $$PIPELINE_ROOT/elemental/ElementalConfig.h \
    $$PIPELINE_ROOT/elemental/ElementalAtlasDirectSink.h \
    $$PIPELINE_ROOT/elemental/ElementalMemoryResult.h \
    $$PIPELINE_ROOT/elemental/ElementalMemoryTransform.h \
    $$PIPELINE_ROOT/elemental/ElementalProcessor.h \
    $$PIPELINE_ROOT/HoloPipeline.h \
    $$PIPELINE_ROOT/PipelineConfig.h \
    $$PIPELINE_ROOT/PipelineContext.h \
    $$PIPELINE_ROOT/PipelineLogger.h \
    $$PIPELINE_ROOT/PipelineTiming.h \
    $$PIPELINE_ROOT/ResultPersistence.h \
    $$PIPELINE_ROOT/ResultSaveSettings.h \
    $$PIPELINE_ROOT/multiview/MultiviewConfig.h \
    $$PIPELINE_ROOT/multiview/MultiviewMemoryResult.h \
    $$PIPELINE_ROOT/multiview/PclMeshOsgBuilder.h \
    $$PIPELINE_ROOT/stages/DepthStage.h \
    $$PIPELINE_ROOT/stages/ElementalStage.h \
    $$PIPELINE_ROOT/stages/MeshStage.h \
    $$PIPELINE_ROOT/stages/ModelStage.h \
    $$PIPELINE_ROOT/stages/MultiviewStage.h

INCLUDEPATH += \
    $$PIPELINE_ROOT \
    $$PIPELINE_ROOT/elemental \
    $$PIPELINE_ROOT/multiview \
    $$PIPELINE_ROOT/stages

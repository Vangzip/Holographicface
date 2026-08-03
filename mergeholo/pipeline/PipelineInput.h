#pragma once

#include <filesystem>
#include <string>

struct MeshMemoryResult;

enum class PipelineInputMode {
    Camera,
    RgbDepth,
    Mesh,
    Multiview
};

struct PipelineInputSelection {
    PipelineInputMode mode = PipelineInputMode::Camera;
    std::filesystem::path directory;

    bool isExternal() const
    {
        return mode != PipelineInputMode::Camera;
    }

    void clear()
    {
        mode = PipelineInputMode::Camera;
        directory.clear();
    }
};

struct MultiviewInputSpec {
    int viewRows = 270;
    int viewCols = 270;
    int viewNameDigits = 3;
    int viewWidth = 150;
    int viewHeight = 150;
};

struct PipelineInputFiles {
    std::filesystem::path rgbPath;
    std::filesystem::path depthPath;
    std::filesystem::path meshPath;
    std::filesystem::path multiviewDirectory;
};

struct PipelineRunPlan {
    bool depth = true;
    bool mesh = true;
    bool model = true;
    bool multiview = true;
    bool elemental = true;
    bool preloadMesh = false;
    bool useFileMultiview = false;
    bool readOnlySource = false;
};

std::string pipelineInputModeName(PipelineInputMode mode);
bool parsePipelineInputMode(const std::string& value, PipelineInputMode* mode);
PipelineRunPlan makePipelineRunPlan(PipelineInputMode mode);

bool resolvePipelineInput(
    const PipelineInputSelection& selection,
    const MultiviewInputSpec& multiviewSpec,
    PipelineInputFiles* files,
    std::string* errorMessage);

bool loadPipelineMeshInput(
    const PipelineInputFiles& files,
    MeshMemoryResult* result,
    std::string* errorMessage);

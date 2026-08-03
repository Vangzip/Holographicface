#pragma once

#include "elemental/ElementalMemoryResult.h"
#include "ModelMoveCameraConfig.h"
#include "multiview/MultiviewMemoryResult.h"
#include "PipelineInput.h"
#include "ResultSaveSettings.h"

#include <filesystem>
#include <string>

struct HoloConfig {
    PipelineInputMode inputMode = PipelineInputMode::Camera;
    std::filesystem::path inputDirectory;
    std::filesystem::path depthInputDir;
    std::filesystem::path depthConfig;
    std::filesystem::path meshConfig;
    std::filesystem::path meshObj;

    std::filesystem::path outputRoot;
    std::filesystem::path meshOutDir;
    std::filesystem::path multiviewOutDir;
    std::filesystem::path elementalOutDir;
    std::filesystem::path logFile;
    ResultSaveSettings saveSettings;
    std::string resultTimestamp;

    std::string modelType = "obj";
    int multiviewAngle = 90;
    int multiviewPer = 3;
    int multiviewResolution = 150;
    int multiviewAtlasSize = 0;

    int targetRows = 150;
    int targetCols = 150;
    int viewRows = 270;
    int viewCols = 270;
    int viewNameDigits = 3;
    int jpgQuality = 100;
    int elementalWriterThreads = 0;
    bool elementalFlipSourceY = false;
    bool elementalFlipViewRows = false;
    ModelMoveCameraConfig multiviewCamera;

    bool runDepthPointCloud = true;
    bool runMesh = true;
    bool runTexturedModel = true;
    bool runMultiview = true;
    bool runElemental = true;
};

struct CliOptions {
    std::filesystem::path configPath;
    std::filesystem::path inputPath;
    std::string stage = "all";
    bool dryRun = false;
    bool showHelp = false;
};

struct StageTiming {
    std::string name;
    double seconds;
    int code;
};

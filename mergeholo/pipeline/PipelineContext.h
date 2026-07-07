#pragma once

#include "elemental/ElementalMemoryResult.h"
#include "ModelMoveCameraConfig.h"
#include "multiview/MultiviewMemoryResult.h"

#include <filesystem>
#include <string>

struct HoloConfig {
    std::filesystem::path depthInputDir;
    std::filesystem::path depthConfig;
    std::filesystem::path meshConfig;
    std::filesystem::path meshObj;

    std::filesystem::path outputRoot;
    std::filesystem::path multiviewOutDir;
    std::filesystem::path elementalOutDir;
    std::filesystem::path logFile;

    std::string modelType = "obj";
    int multiviewAngle = 90;
    int multiviewPer = 3;
    int multiviewResolution = 150;
    int multiviewAtlasSize = 4096;

    int targetRows = 150;
    int targetCols = 150;
    int viewRows = 270;
    int viewCols = 270;
    int viewNameDigits = 3;
    int jpgQuality = 100;
    int elementalWriterThreads = 0;
    bool elementalFlipSourceY = true;
    bool elementalFlipViewRows = true;
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

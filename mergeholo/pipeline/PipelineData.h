#pragma once

#include "memoryFrameSink.h"
#include "ModelMoveCameraConfig.h"
#include "multiviewRenderPlan.h"

#include <cstddef>
#include <filesystem>
#include <memory>
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

struct MultiviewMemoryResult {
    std::unique_ptr<MemoryFrameSink> sink;
    std::unique_ptr<MultiviewRenderPlan> plan;
};

struct ElementalMemoryResult {
    std::unique_ptr<unsigned char[]> pixels;
    size_t imageCount = 0;
    size_t imageBytes = 0;
    size_t totalBytes = 0;
    int rows = 0;
    int cols = 0;
};

#pragma once

#include "PipelineInput.h"
#include "ResultSaveSettings.h"

#include <QString>
#include <QtGlobal>

struct CameraCaptureSettings {
    QString configDirectory;
    int exposureMode = 1;
    int exposureValue = 15000;
    double frameRate = 6.0;
    QString cameraInterface = "571";
    QString cameraType = "Indigo";
    int cameraId = 0;
    int gpuId = 0;
    int missedFrameThreshold = 100;
};

struct PipelineUiSettings {
    QString outputRoot;
    int multiviewAngle = 90;
    int multiviewPer = 3;
    int multiviewResolution = 150;
    int targetRows = 150;
    int targetCols = 150;
    double subjectSize = 2.0;
    double centerX = 0.0;
    double centerY = 0.0;
    double centerZ = 0.0;
    double rotateXDeg = 0.0;
    double rotateZDeg = 0.0;
    int jpgQuality = 100;
    bool captureFlipVertical = true;
    bool elementalFlipSourceY = false;
    bool elementalFlipViewRows = false;
    int atlasSize = 0;
    int writerThreads = 0;
};

struct PointCloudUiSettings {
    double focus = 105.0;
    double disp = 1.0;
    double step = 0.02;
    bool outlierFilterEnabled = false;
};

struct MeshUiSettings {
    int reconstruct = 2;
    int kSearch = 20;
    double searchRadius = 0.01;
    double leafSize = 0.001;
};

struct ProcessingSettings {
    PipelineInputSelection input;
    ResultSaveSettings saveResults;
    PipelineUiSettings pipeline;
    PointCloudUiSettings pointCloud;
    MeshUiSettings mesh;
    CameraCaptureSettings camera;
};

ProcessingSettings defaultProcessingSettings(
    const QString& projectRoot,
    const QString& cameraConfigDirectory);

double subjectSizeFromDistanceScale(double distanceScale);
double distanceScaleFromSubjectSize(double subjectSize);
int viewCountPerAxis(const ProcessingSettings& settings);
qint64 elementalImageCount(const ProcessingSettings& settings);
QString validateProcessingSettings(const ProcessingSettings& settings);

#pragma once

#include "ProcessingSettings.h"

#include <QString>

struct CaptureSessionOptions {
    QString saveRoot;
    CameraCaptureSettings cameraSettings;
    int minFreeSpaceGb = 50;
    int saveIntervalMs = 100;
    int maxFrames = 0;
    int durationSeconds = 0;
    bool showPreview = true;
};

int runCaptureSession(const CaptureSessionOptions& options);

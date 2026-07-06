#pragma once

#include <QString>

struct CaptureSessionOptions {
    QString saveRoot;
    QString cameraConfigPath;
    int minFreeSpaceGb = 50;
    int saveIntervalMs = 100;
    int maxFrames = 0;
    int durationSeconds = 0;
    bool showPreview = true;
};

int runCaptureSession(const CaptureSessionOptions& options);

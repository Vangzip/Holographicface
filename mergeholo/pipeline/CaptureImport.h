#pragma once

#include <QString>

struct CaptureImportOptions {
    QString captureRoot;
    QString pipelineInputDir;
    bool overwrite = true;
};

int importCaptureForPipeline(const CaptureImportOptions& options);

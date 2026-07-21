#pragma once

#include "ProcessingSettings.h"

#include <QString>

struct ProcessingSettingsPaths {
    QString projectRoot;
    QString pipelineConfig;
    QString pointCloudConfig;
    QString meshConfig;
    QString cameraConfig;

    static ProcessingSettingsPaths fromProjectRoot(const QString& projectRoot);
};

bool loadProcessingSettings(
    const ProcessingSettingsPaths& paths,
    ProcessingSettings* settings,
    QString* errorMessage = nullptr);

bool saveProcessingSettings(
    const ProcessingSettingsPaths& paths,
    const ProcessingSettings& settings,
    QString* errorMessage = nullptr);

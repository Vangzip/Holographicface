#pragma once

#include <QtGlobal>
#include <QString>
#include <QVector>

#include "PrintConfig.h"
#include "PrintHardwareProfile.h"

struct V2RowPlan {
    int row = 0;
    bool reverse = false;
    qint64 yStart = 0;
    qint64 yTarget = 0;
    qint64 constantBegin = 0;
    qint64 exposureBegin = 0;
    qint64 compareBegin = 0;
    qint64 compareEnd = 0;
    QVector<int> logicalFrameOrder;
    int startDelayFrames = 0;
    int holdFramesAfterPresent = 0;
};

struct V2PrintPlan {
    qint64 stepPulse = 0;
    qint64 accelerationPulse = 0;
    qint64 exposurePulse = 0;
    qint64 totalPulse = 0;
    int framesPerImage = 0;
    QVector<V2RowPlan> rows;
};

V2PrintPlan buildV2PrintPlan(
    const Print9030Config& config,
    const PrintHardwareProfile& profile,
    double refreshHz,
    QString* errorMessage = nullptr);

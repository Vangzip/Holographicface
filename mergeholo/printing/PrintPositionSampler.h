#pragma once

#include "PrintConfig.h"

#include <QPointF>
#include <QtGlobal>

class PrintPositionSampler
{
public:
    explicit PrintPositionSampler(qint64 intervalMs = 100);

    bool isDue(qint64 nowMs);
    static bool toMillimeters(int xPulses, int yPulses,
        const PrintAxisConfig& xAxis, const PrintAxisConfig& yAxis,
        QPointF* position);

private:
    qint64 intervalMs_ = 100;
    qint64 lastSampleMs_ = -1;
};

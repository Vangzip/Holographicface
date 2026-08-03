#include "PrintPositionSampler.h"

#include <cmath>

PrintPositionSampler::PrintPositionSampler(qint64 intervalMs)
    : intervalMs_(qMax<qint64>(1, intervalMs))
{
}

bool PrintPositionSampler::isDue(qint64 nowMs)
{
    if (lastSampleMs_ < 0 || nowMs < lastSampleMs_
        || nowMs - lastSampleMs_ >= intervalMs_) {
        lastSampleMs_ = nowMs;
        return true;
    }
    return false;
}

bool PrintPositionSampler::toMillimeters(int xPulses, int yPulses,
    const PrintAxisConfig& xAxis, const PrintAxisConfig& yAxis, QPointF* position)
{
    if (!position) return false;
    const double xScale = static_cast<double>(xAxis.subdivision) * xAxis.resolution;
    const double yScale = static_cast<double>(yAxis.subdivision) * yAxis.resolution;
    if (!std::isfinite(xScale) || !std::isfinite(yScale) || xScale <= 0.0 || yScale <= 0.0) {
        return false;
    }
    *position = QPointF(xPulses / xScale, yPulses / yScale);
    return true;
}

#include "LegacyPrintTiming.h"

#include <QtMath>

#include <cmath>
#include <limits>

namespace {

void setMessage(QString* errorMessage, const QString& message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}

bool fitsLong(qint64 value)
{
    return value >= std::numeric_limits<long>::min() && value <= std::numeric_limits<long>::max();
}

} // namespace

bool calculateLegacyPrintTiming(
    const Print9030Config& config,
    LegacyPrintTiming* timing,
    QString* errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!timing) {
        setMessage(errorMessage, "Legacy timing destination is not available.");
        return false;
    }
    *timing = {};

    const PrintAxisConfig& axisY = config.axisY;
    const PrintAxisConfig& axisX = config.axisX;
    if (config.main.gridRows <= 0 || config.main.gridColumns <= 0
        || axisY.subdivision <= 0 || axisY.resolution <= 0
        || axisX.subdivision <= 0 || axisX.resolution <= 0
        || axisY.speedOfMovement <= 0 || axisY.acceleratedVelocity <= 0
        || axisY.startSpeed < 0 || axisY.startSpeed > axisY.speedOfMovement
        || config.main.columnSpacingMm == 0.0) {
        setMessage(errorMessage, "Legacy 9030 scan configuration has invalid dimensions or axis speeds.");
        return false;
    }

    const double accelerationTime = static_cast<double>(axisY.speedOfMovement - axisY.startSpeed)
        / static_cast<double>(axisY.acceleratedVelocity);
    const qint64 accelerationPulse = qRound64(
        (static_cast<double>(axisY.startSpeed) + static_cast<double>(axisY.speedOfMovement))
        * accelerationTime * 0.5);
    const qint64 yStepPulse = qRound64(
        static_cast<double>(axisY.subdivision) * static_cast<double>(axisY.resolution)
        * std::abs(config.main.columnSpacingMm));
    const qint64 xStepPulse = qRound64(
        static_cast<double>(axisX.subdivision) * static_cast<double>(axisX.resolution)
        * std::abs(config.main.columnSpacingMm));
    const qint64 exposurePulse = yStepPulse * config.main.gridColumns;
    const qint64 totalPulse = exposurePulse + 2 * accelerationPulse + config.main.addTempPulse;
    if (accelerationPulse < 0 || yStepPulse <= 0 || xStepPulse <= 0
        || exposurePulse <= 0 || totalPulse <= 0
        || !fitsLong(accelerationPulse) || !fitsLong(yStepPulse) || !fitsLong(xStepPulse)
        || !fitsLong(exposurePulse) || !fitsLong(totalPulse)) {
        setMessage(errorMessage, "Legacy 9030 scan pulse calculation is outside the control card range.");
        return false;
    }

    const double rawFramesPerImage = (static_cast<double>(yStepPulse) * 1000000.0
        / static_cast<double>(axisY.speedOfMovement)) / (1000000.0 / 60.0);
    const qint64 roundedFramesPerImage = qRound64(rawFramesPerImage);
    if (roundedFramesPerImage <= 0 || roundedFramesPerImage > std::numeric_limits<int>::max()
        || std::abs(rawFramesPerImage - static_cast<double>(roundedFramesPerImage)) > 0.05) {
        setMessage(errorMessage, "Legacy 9030 scan does not fit an integer number of VBlank frames per image.");
        return false;
    }

    timing->yStepPulse = static_cast<long>(yStepPulse);
    timing->xStepPulse = static_cast<long>(xStepPulse);
    timing->accelerationPulse = static_cast<long>(accelerationPulse);
    timing->exposurePulse = static_cast<long>(exposurePulse);
    timing->addTempPulse = config.main.addTempPulse;
    timing->totalPulse = static_cast<long>(totalPulse);
    timing->framesPerImage = static_cast<int>(roundedFramesPerImage);
    return true;
}

LegacyRowPlan makeLegacyRowPlan(const LegacyPrintTiming& timing, long yStart, bool reverse)
{
    LegacyRowPlan plan;
    plan.yStart = yStart;
    plan.reverse = reverse;
    if (reverse) {
        plan.yTarget = yStart - timing.totalPulse;
        plan.exposureBegin = yStart - timing.accelerationPulse - timing.addTempPulse + timing.yStepPulse;
        plan.exposureEnd = plan.exposureBegin - timing.exposurePulse;
        plan.compareBegin = std::numeric_limits<long>::min();
        plan.compareEnd = plan.exposureBegin;
    } else {
        plan.yTarget = yStart + timing.totalPulse;
        plan.exposureBegin = yStart + timing.accelerationPulse;
        plan.exposureEnd = plan.exposureBegin + timing.exposurePulse;
        plan.compareBegin = plan.exposureBegin;
        plan.compareEnd = std::numeric_limits<long>::max();
    }
    return plan;
}

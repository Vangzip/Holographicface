#include "V2PrintTiming.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr qint64 kMaximumPlannedFrames = 3000000;
constexpr int kMaximumPlannedRows = 100000;

V2PrintPlan fail(QString* errorMessage, const QString& message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return {};
}

bool checkedAdd(qint64 left, qint64 right, qint64* result)
{
    const qint64 maximum = std::numeric_limits<qint64>::max();
    const qint64 minimum = std::numeric_limits<qint64>::min();
    if ((right > 0 && left > maximum - right)
        || (right < 0 && left < minimum - right)) {
        return false;
    }
    *result = left + right;
    return true;
}

bool checkedSubtract(qint64 left, qint64 right, qint64* result)
{
    const qint64 maximum = std::numeric_limits<qint64>::max();
    const qint64 minimum = std::numeric_limits<qint64>::min();
    if ((right > 0 && left < minimum + right)
        || (right < 0 && left > maximum + right)) {
        return false;
    }
    *result = left - right;
    return true;
}

bool checkedMultiplyNonNegative(qint64 left, qint64 right, qint64* result)
{
    if (left < 0 || right < 0) {
        return false;
    }
    if (left != 0 && right > std::numeric_limits<qint64>::max() / left) {
        return false;
    }
    *result = left * right;
    return true;
}

bool isSdkPosition(qint64 value)
{
    return value >= std::numeric_limits<qint32>::min()
        && value <= std::numeric_limits<qint32>::max();
}

bool isSupportedVBlankCount(int frames)
{
    return frames == 1 || frames == 2 || frames == 3
        || frames == 4 || frames == 6;
}

bool finite(double value)
{
    return std::isfinite(value);
}

bool validateFiniteMainConfig(const PrintMainConfig& main, QString* field)
{
    struct NamedValue {
        const char* name;
        double value;
    };
    const NamedValue values[] = {
        {"moveAdjustMm", main.moveAdjustMm},
        {"rowSpacingMm", main.rowSpacingMm},
        {"columnSpacingMm", main.columnSpacingMm},
        {"delaySeconds", main.delaySeconds},
        {"exposureSeconds", main.exposureSeconds},
        {"widthScale", main.widthScale},
        {"heightScale", main.heightScale}
    };
    for (const NamedValue& item : values) {
        if (!finite(item.value)) {
            *field = QString::fromLatin1(item.name);
            return false;
        }
    }
    return true;
}

bool checkedRoundedInt(double value, int* result)
{
    if (!finite(value)
        || value < static_cast<double>(std::numeric_limits<int>::min())
        || value > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }
    *result = static_cast<int>(value);
    return true;
}

bool checkedRoundedQint64(double value, qint64* result)
{
    if (!finite(value)) {
        return false;
    }
    const long double promoted = static_cast<long double>(value);
    if (promoted < static_cast<long double>(std::numeric_limits<qint64>::min())
        || promoted > static_cast<long double>(std::numeric_limits<qint64>::max())) {
        return false;
    }
    *result = static_cast<qint64>(value);
    return true;
}

} // namespace

V2PrintPlan buildV2PrintPlan(
    const Print9030Config& config,
    const PrintHardwareProfile& profile,
    double refreshHz,
    QString* errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    QString profileError;
    if (!validatePrintHardwareProfile(profile, &profileError)) {
        return fail(errorMessage, "IMC60G hardware profile is invalid: " + profileError);
    }
    if (!finite(refreshHz) || refreshHz <= 0.0) {
        return fail(errorMessage,
            QString("refreshHz must be finite and positive; value=%1").arg(refreshHz));
    }

    QString nonFiniteField;
    if (!validateFiniteMainConfig(config.main, &nonFiniteField)) {
        return fail(errorMessage,
            QString("%1 must be finite.").arg(nonFiniteField));
    }
    const struct {
        const char* name;
        const PrintAxisConfig* axis;
    } axes[] = {
        {"axisX", &config.axisX},
        {"axisY", &config.axisY},
        {"axisZ", &config.axisZ},
        {"axisW", &config.axisW}
    };
    for (const auto& item : axes) {
        if (!finite(item.axis->maxDistance)) {
            return fail(errorMessage,
                QString("%1.maxDistance must be finite; value=%2")
                    .arg(QString::fromLatin1(item.name))
                    .arg(item.axis->maxDistance));
        }
    }
    if (config.main.gridRows <= 0 || config.main.gridColumns <= 0) {
        return fail(errorMessage,
            QString("grid rows and columns must be positive; rows=%1 columns=%2")
                .arg(config.main.gridRows)
                .arg(config.main.gridColumns));
    }

    qint64 requiredFrames = 0;
    if (!checkedMultiplyNonNegative(
            config.main.gridRows, config.main.gridColumns, &requiredFrames)) {
        return fail(errorMessage, "grid rows*columns arithmetic overflow.");
    }
    if (config.main.gridRows > kMaximumPlannedRows
        || requiredFrames > kMaximumPlannedFrames) {
        return fail(errorMessage,
            QString("grid allocation is too large; rows=%1 columns=%2 frames=%3 limit=%4")
                .arg(config.main.gridRows)
                .arg(config.main.gridColumns)
                .arg(requiredFrames)
                .arg(kMaximumPlannedFrames));
    }
    if (config.main.delaySeconds < 0.0) {
        return fail(errorMessage,
            QString("delaySeconds cannot be negative; value=%1")
                .arg(config.main.delaySeconds));
    }
    if (config.main.exposureSeconds <= 0.0) {
        return fail(errorMessage,
            QString("exposureSeconds must be positive; value=%1")
                .arg(config.main.exposureSeconds));
    }
    if (config.main.widthScale <= 0.0 || config.main.heightScale <= 0.0) {
        return fail(errorMessage,
            QString("widthScale and heightScale must be positive; values=%1,%2")
                .arg(config.main.widthScale)
                .arg(config.main.heightScale));
    }
    if (config.main.addTempPulse < 0 || config.main.leadPulse < 0) {
        return fail(errorMessage,
            QString("addTempPulse and leadPulse must be non-negative; values=%1,%2")
                .arg(config.main.addTempPulse)
                .arg(config.main.leadPulse));
    }

    const PrintAxisConfig& y = config.axisY;
    if (y.subdivision <= 0 || y.resolution <= 0) {
        return fail(errorMessage,
            QString("Y subdivision and resolution must be positive; values=%1,%2")
                .arg(y.subdivision)
                .arg(y.resolution));
    }
    if (y.speedOfMovement <= 0) {
        return fail(errorMessage,
            QString("Y speedOfMovement must be positive; value=%1")
                .arg(y.speedOfMovement));
    }
    if (y.acceleratedVelocity <= 0) {
        return fail(errorMessage,
            QString("Y acceleration must be positive; value=%1")
                .arg(y.acceleratedVelocity));
    }
    if (y.startSpeed < 0 || y.startSpeed > y.speedOfMovement) {
        return fail(errorMessage,
            QString("Y startSpeed must be between zero and speedOfMovement; value=%1 speed=%2")
                .arg(y.startSpeed)
                .arg(y.speedOfMovement));
    }
    if (y.stopSpeed < 0 || y.stopSpeed > y.speedOfMovement) {
        return fail(errorMessage,
            QString("Y stopSpeed must be between zero and speedOfMovement; value=%1 speed=%2")
                .arg(y.stopSpeed)
                .arg(y.speedOfMovement));
    }
    if (!finite(y.maxDistance) || y.maxDistance <= 0.0) {
        return fail(errorMessage,
            QString("Y maxDistance must be finite and positive; value=%1")
                .arg(y.maxDistance));
    }

    qint64 pulsesPerMillimeter = 0;
    if (!checkedMultiplyNonNegative(
            y.subdivision, y.resolution, &pulsesPerMillimeter)) {
        return fail(errorMessage, "stepPulse subdivision*resolution overflow.");
    }
    const long double rawStep =
        static_cast<long double>(pulsesPerMillimeter)
        * std::fabs(static_cast<long double>(config.main.columnSpacingMm));
    if (!std::isfinite(rawStep)
        || rawStep > static_cast<long double>(std::numeric_limits<qint64>::max())) {
        return fail(errorMessage,
            QString("stepPulse arithmetic overflow; columnSpacingMm=%1")
                .arg(config.main.columnSpacingMm));
    }
    const qint64 stepPulse = static_cast<qint64>(rawStep);
    if (stepPulse != static_cast<qint64>(profile.printStepPulse)) {
        return fail(errorMessage,
            QString("stepPulse must match the IMC60G 1000-pulse basis; calculated=%1 expected=%2")
                .arg(stepPulse)
                .arg(profile.printStepPulse));
    }

    const double startSpeed = static_cast<double>(y.startSpeed);
    const double speed = static_cast<double>(y.speedOfMovement);
    const double acceleration = static_cast<double>(y.acceleratedVelocity);
    const double accelerationTime = (speed - startSpeed) / acceleration;
    const double rawAccelerationPulse =
        (startSpeed + speed) * accelerationTime * 0.5;
    qint64 accelerationPulse = 0;
    if (!checkedRoundedQint64(rawAccelerationPulse + 0.5, &accelerationPulse)) {
        return fail(errorMessage,
            QString("accelerationPulse arithmetic overflow; raw=%1")
                .arg(rawAccelerationPulse, 0, 'g', 17));
    }
    if (!isSdkPosition(accelerationPulse)) {
        return fail(errorMessage,
            QString("accelerationPulse is outside int32 SDK range; value=%1")
                .arg(accelerationPulse));
    }

    qint64 exposurePulse = 0;
    if (!checkedMultiplyNonNegative(
            stepPulse, config.main.gridColumns, &exposurePulse)) {
        return fail(errorMessage, "exposurePulse stepPulse*columns arithmetic overflow.");
    }
    if (!isSdkPosition(exposurePulse)) {
        return fail(errorMessage,
            QString("exposurePulse is outside int32 SDK range; value=%1")
                .arg(exposurePulse));
    }

    qint64 twoAccelerationPulses = 0;
    if (!checkedMultiplyNonNegative(accelerationPulse, 2, &twoAccelerationPulses)) {
        return fail(errorMessage, "totalPulse 2*accelerationPulse arithmetic overflow.");
    }
    qint64 totalPulse = 0;
    if (!checkedAdd(exposurePulse, twoAccelerationPulses, &totalPulse)
        || !checkedAdd(totalPulse, config.main.addTempPulse, &totalPulse)) {
        return fail(errorMessage, "totalPulse arithmetic overflow.");
    }
    if (totalPulse <= 0 || !isSdkPosition(totalPulse)) {
        return fail(errorMessage,
            QString("totalPulse is outside positive int32 SDK range; value=%1")
                .arg(totalPulse));
    }

    qint64 constantWidthPulse = 0;
    if (!checkedAdd(exposurePulse, config.main.addTempPulse, &constantWidthPulse)) {
        return fail(errorMessage, "constant segment pulse arithmetic overflow.");
    }

    qint64 reverseDelayPulse = config.main.addTempPulse;
    if (!checkedSubtract(
            reverseDelayPulse, profile.reverseFixedPulse, &reverseDelayPulse)
        || !checkedSubtract(
            reverseDelayPulse, profile.forwardDelayPulse, &reverseDelayPulse)
        || !checkedSubtract(
            reverseDelayPulse, config.main.leadPulse, &reverseDelayPulse)) {
        return fail(errorMessage, "reverse display delay arithmetic overflow.");
    }

    const double constantSegmentUs =
        static_cast<double>(stepPulse) * 1000000.0 / speed;
    const double displayFrameUs = 1000000.0 / refreshHz;
    const double rawFramesPerImage = constantSegmentUs / displayFrameUs;
    int framesPerImage = 0;
    if (!checkedRoundedInt(rawFramesPerImage + 0.5, &framesPerImage)) {
        return fail(errorMessage,
            QString("VBlank framesPerImage arithmetic overflow; raw=%1")
                .arg(rawFramesPerImage, 0, 'g', 17));
    }
    if (framesPerImage < 1) {
        framesPerImage = 1;
    }
    const double frameFitError =
        std::fabs(rawFramesPerImage - static_cast<double>(framesPerImage));
    if (!finite(constantSegmentUs) || constantSegmentUs <= 0.0
        || !finite(frameFitError) || frameFitError > 0.05) {
        return fail(errorMessage,
            QString("VBlank cadence does not fit within 0.05 frame; raw=%1 rounded=%2 error=%3")
                .arg(rawFramesPerImage, 0, 'g', 17)
                .arg(framesPerImage)
                .arg(frameFitError, 0, 'g', 17));
    }
    if (!isSupportedVBlankCount(framesPerImage)) {
        return fail(errorMessage,
            QString("unsupported IMC60G VBlank sequence count; framesPerImage=%1")
                .arg(framesPerImage));
    }

    const double pulsePerRefreshFrame = speed / refreshHz;
    if (!finite(pulsePerRefreshFrame) || pulsePerRefreshFrame <= 0.0) {
        return fail(errorMessage,
            QString("pulsePerRefreshFrame must be finite and positive; value=%1")
                .arg(pulsePerRefreshFrame, 0, 'g', 17));
    }

    auto delayFrames = [&](qint64 delayPulse, int* frames) -> bool {
        const double rounded =
            static_cast<double>(delayPulse) / pulsePerRefreshFrame + 0.5;
        if (!checkedRoundedInt(rounded, frames)) {
            return false;
        }
        if (*frames < 0) {
            *frames = 0;
        }

        qint64 effectivePulse = 0;
        if (!checkedRoundedQint64(
                static_cast<double>(*frames) * pulsePerRefreshFrame + 0.5,
                &effectivePulse)) {
            return false;
        }
        return isSdkPosition(effectivePulse);
    };

    int forwardDelayFrames = 0;
    int reverseDelayFrames = 0;
    if (!delayFrames(profile.forwardDelayPulse, &forwardDelayFrames)
        || !delayFrames(reverseDelayPulse, &reverseDelayFrames)) {
        return fail(errorMessage,
            QString("startDelayFrames arithmetic overflow; forwardPulse=%1 reversePulse=%2")
                .arg(profile.forwardDelayPulse)
                .arg(reverseDelayPulse));
    }

    V2PrintPlan plan;
    plan.stepPulse = stepPulse;
    plan.accelerationPulse = accelerationPulse;
    plan.exposurePulse = exposurePulse;
    plan.totalPulse = totalPulse;
    plan.framesPerImage = framesPerImage;
    plan.rows.reserve(config.main.gridRows);

    qint64 currentY = 0;
    for (int rowIndex = 0; rowIndex < config.main.gridRows; ++rowIndex) {
        // In active V2 start_new, bForward starts false and becomes true on
        // alternating rows. Its true branch is physical reverse Y movement.
        const bool reverse = (rowIndex % 2) != 0;
        V2RowPlan row;
        row.row = rowIndex;
        row.reverse = reverse;
        row.yStart = currentY;

        if (!checkedAdd(
                currentY, reverse ? -totalPulse : totalPulse, &row.yTarget)
            || !checkedAdd(
                currentY,
                reverse ? -accelerationPulse : accelerationPulse,
                &row.constantBegin)) {
            return fail(errorMessage,
                QString("row %1 Y target arithmetic overflow.").arg(rowIndex));
        }

        qint64 constantEnd = 0;
        if (!checkedAdd(
                row.constantBegin,
                reverse ? -constantWidthPulse : constantWidthPulse,
                &constantEnd)) {
            return fail(errorMessage,
                QString("row %1 constant end arithmetic overflow.").arg(rowIndex));
        }

        qint64 exposureBeginWithoutOffset = row.constantBegin;
        if (reverse
            && (!checkedSubtract(
                    exposureBeginWithoutOffset,
                    config.main.addTempPulse,
                    &exposureBeginWithoutOffset)
                || !checkedAdd(
                    exposureBeginWithoutOffset,
                    stepPulse,
                    &exposureBeginWithoutOffset))) {
            return fail(errorMessage,
                QString("row %1 reverse exposure begin arithmetic overflow.")
                    .arg(rowIndex));
        }
        if (!checkedAdd(
                exposureBeginWithoutOffset,
                profile.exposureOffsetPulse,
                &row.exposureBegin)) {
            return fail(errorMessage,
                QString("row %1 exposure offset arithmetic overflow.")
                    .arg(rowIndex));
        }

        qint64 exposureEnd = 0;
        if (!checkedAdd(
                row.exposureBegin,
                reverse ? -exposurePulse : exposurePulse,
                &exposureEnd)) {
            return fail(errorMessage,
                QString("row %1 exposure end arithmetic overflow.")
                    .arg(rowIndex));
        }

        row.compareBegin = std::min(row.exposureBegin, exposureEnd);
        row.compareEnd = std::max(row.exposureBegin, exposureEnd);

        const struct {
            const char* name;
            qint64 value;
        } positions[] = {
            {"yStart", row.yStart},
            {"yTarget", row.yTarget},
            {"constantBegin", row.constantBegin},
            {"constantEnd", constantEnd},
            {"exposureBegin", row.exposureBegin},
            {"exposureEnd", exposureEnd},
            {"compareBegin", row.compareBegin},
            {"compareEnd", row.compareEnd}
        };
        for (const auto& position : positions) {
            if (!isSdkPosition(position.value)) {
                return fail(errorMessage,
                    QString("row %1 %2 is outside int32 SDK range; value=%3")
                        .arg(rowIndex)
                        .arg(QString::fromLatin1(position.name))
                        .arg(position.value));
            }
        }

        row.logicalFrameOrder.reserve(config.main.gridColumns);
        for (int column = 0; column < config.main.gridColumns; ++column) {
            row.logicalFrameOrder.append(
                reverse ? config.main.gridColumns - 1 - column : column);
        }
        row.startDelayFrames =
            reverse ? reverseDelayFrames : forwardDelayFrames;
        // Task 5 describes the production IMC path after presenter preflight;
        // active V2's verified-output VBlank branch holds framesPerImage-1.
        row.holdFramesAfterPresent = framesPerImage - 1;

        plan.rows.append(row);
        currentY = row.yTarget;
    }

    return plan;
}

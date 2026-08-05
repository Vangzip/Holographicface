#include "PrintHardwarePreflight.h"

#include <limits>

namespace {

PrintPreflightResult fail(PreflightFault fault, const QString& detail)
{
    PrintPreflightResult result;
    result.fault = fault;
    result.detail = detail;
    return result;
}

bool sdkPosition(qint64 value)
{
    return value >= std::numeric_limits<qint32>::min()
        && value <= std::numeric_limits<qint32>::max();
}

} // namespace

PrintHardwarePreflight::PrintHardwarePreflight(IMotionController& motion,
    IExposureController& exposure, IPrintFramePresenter& presenter)
    : motion_(motion)
    , exposure_(exposure)
    , presenter_(presenter)
{
}

PrintPreflightResult PrintHardwarePreflight::check(
    const PrintJobSnapshot& job, bool dynamicOnly)
{
    QString detail;
    const PrintMotionReadiness motion = motion_.printReadiness(&detail);
    if (!motion.sdkRuntimeReady) return fail(PreflightFault::SdkRuntime, "x64 IMC60G SDK/runtime is not ready. " + detail);
    if (!motion.cardReady) return fail(PreflightFault::Card, "IMC60G Card0 is not connected. " + detail);
    if (!motion.ethercatReady) return fail(PreflightFault::Ethercat, "EtherCAT master is not ready. " + detail);
    if (!motion.axisMappingLocked || job.profile.cardIndex != 0
        || job.profile.axisX != 1 || job.profile.axisY != 0) {
        return fail(PreflightFault::AxisMapping, "Hardware mapping must be locked to Card0, X=1, Y=0.");
    }
    if (!motion.servosReady) return fail(PreflightFault::Servo, "X/Y Servo is not ready or has an alarm. " + detail);
    if (!motion.emergencyClear) return fail(PreflightFault::Emergency, "Software emergency state is not clear. " + detail);
    if (!motion.axesHomed || !motion.axesStopped) return fail(PreflightFault::Homing, "X/Y home and stopped verification failed. " + detail);

    const PrintExposureReadiness exposure = exposure_.printReadiness(&detail);
    if (!exposure.profileMatches) return fail(PreflightFault::ExposureProfile, "SV660N DO1 position-compare profile does not match. " + detail);
    if (!exposure.safeBaseline || exposure_.isArmed()) return fail(PreflightFault::ExposureBaseline, "SV660N exposure safe/disarmed baseline is not verified. " + detail);

    if (!dynamicOnly) {
        QString profileError;
        if (!validatePrintHardwareProfile(job.profile, &profileError)) {
            return fail(PreflightFault::AxisMapping, "IMC60G production hardware profile is invalid: " + profileError);
        }
        if (job.config.main.gridRows <= 0 || job.config.main.gridColumns <= 0
            || job.plan.rows.size() != job.config.main.gridRows) {
            return fail(PreflightFault::TimingPlan, "V2 print plan row count is invalid.");
        }
        for (int rowIndex = 0; rowIndex < job.plan.rows.size(); ++rowIndex) {
            const V2RowPlan& row = job.plan.rows.at(rowIndex);
            if (row.logicalFrameOrder.size() != job.config.main.gridColumns
                || row.row != rowIndex
                || !sdkPosition(row.yStart) || !sdkPosition(row.yTarget)
                || !sdkPosition(row.compareBegin) || !sdkPosition(row.compareEnd)
                || row.holdFramesAfterPresent < 0 || row.startDelayFrames < 0) {
                return fail(PreflightFault::TimingPlan, "V2 row plan or SDK int32 position range is invalid.");
            }
            QVector<bool> seen(job.config.main.gridColumns, false);
            for (int logicalFrame : row.logicalFrameOrder) {
                if (logicalFrame < 0
                    || logicalFrame >= job.config.main.gridColumns
                    || seen.at(logicalFrame)) {
                    return fail(PreflightFault::TimingPlan,
                        "V2 row frame order must contain each column exactly once.");
                }
                seen[logicalFrame] = true;
            }
        }
    }

    const PrintPresenterReadiness presenter = presenter_.printReadiness(&detail);
    if (!presenter.secondScreenAttached) return fail(PreflightFault::SecondScreen, "Required second screen is not attached. " + detail);
    if (!presenter.presenterAvailable) return fail(PreflightFault::Presenter, "D3D/DXGI presenter is not ready. " + detail);
    if (!presenter.vblankReady || !presenter.generationCurrent) return fail(PreflightFault::VBlank, "Physical VBlank/generation is not ready. " + detail);

    PrintPreflightResult success;
    success.ok = true;
    return success;
}

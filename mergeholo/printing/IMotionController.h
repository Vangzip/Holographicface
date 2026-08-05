#pragma once

#include "PrintConfig.h"

#include <QString>
#include <QtGlobal>

#include <atomic>

struct PrintMotionReadiness {
    bool sdkRuntimeReady = false;
    bool cardReady = false;
    bool ethercatReady = false;
    bool axisMappingLocked = false;
    bool servosReady = false;
    bool emergencyClear = false;
    bool axesHomed = false;
    bool axesStopped = false;
};

// Best-effort telemetry only. A failed sample must never alter print control.
struct PrintMappedAxisTelemetry {
    bool available = false;
    qint32 xPlannedPosition = 0;
    qint32 xEncoderPosition = 0;
    qint32 yPlannedPosition = 0;
    qint32 yEncoderPosition = 0;
};

// Narrow, already-connected print-motion service. Connection, EtherCAT,
// Servo, emergency release and homing are deliberately absent.
class IMotionController
{
public:
    virtual ~IMotionController() = default;

    virtual PrintMotionReadiness printReadiness(QString* errorMessage = nullptr) const = 0;
    virtual bool beginPrint(QString* errorMessage = nullptr) = 0;
    virtual void endPrint() = 0;

    virtual bool startYScan(qint32 absoluteTarget, const PrintAxisConfig& config,
        QString* errorMessage = nullptr) = 0;
    // V2 prepares the Y profile and target before taking the row VBlank
    // anchor. Only startPreparedYScan may run after that anchor.
    virtual bool prepareYScan(qint32 absoluteTarget, const PrintAxisConfig& config,
        QString* errorMessage = nullptr)
    {
        Q_UNUSED(absoluteTarget);
        Q_UNUSED(config);
        if (errorMessage) *errorMessage = QStringLiteral("Prepared Y scans are unsupported.");
        return false;
    }
    virtual bool startPreparedYScan(QString* errorMessage = nullptr)
    {
        if (errorMessage) *errorMessage = QStringLiteral("No prepared Y scan is available.");
        return false;
    }
    virtual bool waitYStopped(int timeoutMs, const std::atomic_bool& cancelRequested,
        QString* errorMessage = nullptr) = 0;
    virtual bool stepX(qint32 relativePulses, const PrintAxisConfig& config,
        QString* errorMessage = nullptr) = 0;
    virtual bool waitXStopped(int timeoutMs, const std::atomic_bool& cancelRequested,
        QString* errorMessage = nullptr) = 0;

    virtual bool stopMappedAxes(QString* errorMessage = nullptr) = 0;
    virtual bool waitMappedAxesStopped(int timeoutMs, QString* errorMessage = nullptr) = 0;
    virtual bool returnToLogicalZero(const PrintAxisConfig& xConfig,
        const PrintAxisConfig& yConfig, int timeoutMs,
        QString* errorMessage = nullptr) = 0;
    virtual bool verifyLogicalZero(QString* errorMessage = nullptr) = 0;

    // Called only for print-flow diagnostics. Implementations that cannot
    // provide a non-intrusive sample retain the unavailable default.
    virtual PrintMappedAxisTelemetry sampleMappedAxisTelemetry()
    {
        return {};
    }

    // V2's print thread reads only logical Y's planned position through
    // Motion_ReadAxisPos(1). Keep that hot path separate from optional
    // multi-axis diagnostic telemetry.
    virtual bool readMappedYPlannedPosition(qint32* position,
        QString* errorMessage = nullptr)
    {
        const PrintMappedAxisTelemetry telemetry = sampleMappedAxisTelemetry();
        if (!telemetry.available) {
            if (errorMessage) *errorMessage = QStringLiteral("Y planned position is unavailable.");
            return false;
        }
        if (position) *position = telemetry.yPlannedPosition;
        if (errorMessage) errorMessage->clear();
        return true;
    }
};

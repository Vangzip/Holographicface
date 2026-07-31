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
};

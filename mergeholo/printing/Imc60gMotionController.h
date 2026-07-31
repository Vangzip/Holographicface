#pragma once

#include "PrintConfig.h"
#include "PrintHardwareProfile.h"
#include "IMotionController.h"

#include <QMutex>
#include <QString>
#include <QtGlobal>

#include <atomic>
#include <memory>

class IImc60gApi;

class IImc60gClock {
public:
    virtual ~IImc60gClock() = default;
    virtual qint64 nowMs() const = 0;
    virtual void sleepMs(unsigned long milliseconds) = 0;
};

enum class Imc60gConnectionState {
    Disconnected,
    Connecting,
    Homing,
    Ready,
    Fault
};

struct Imc60gAxisSnapshot {
    PrintHardwareProfile::LogicalAxis logicalAxis = PrintHardwareProfile::LogicalAxis::X;
    short physicalAxis = -1;
    unsigned int status = 0;
    unsigned int stopReason = 0;
    int plannedPosition = 0;
    int encoderPosition = 0;
};

class Imc60gMotionController final : public IMotionController {
public:
    Imc60gMotionController(IImc60gApi* api, const PrintHardwareProfile& profile,
        IImc60gClock* clock = nullptr);
    ~Imc60gMotionController();

    Imc60gConnectionState state() const;
    bool connectAndHome(QString* errorMessage = nullptr);
    bool disconnect(QString* errorMessage = nullptr);
    void requestCancellation();

    bool moveRelative(PrintHardwareProfile::LogicalAxis logicalAxis, double millimeters,
        const PrintAxisConfig& axisConfig, QString* errorMessage = nullptr);
    bool stopAxis(PrintHardwareProfile::LogicalAxis logicalAxis,
        QString* errorMessage = nullptr);
    bool readSnapshot(PrintHardwareProfile::LogicalAxis logicalAxis,
        Imc60gAxisSnapshot* snapshot, QString* errorMessage = nullptr);
    bool isReadyForPrint() const;
    void setPrintActive(bool active);

    PrintMotionReadiness printReadiness(QString* errorMessage = nullptr) const override;
    bool beginPrint(QString* errorMessage = nullptr) override;
    void endPrint() override;
    bool startYScan(qint32 absoluteTarget, const PrintAxisConfig& config,
        QString* errorMessage = nullptr) override;
    bool waitYStopped(int timeoutMs, const std::atomic_bool& cancelRequested,
        QString* errorMessage = nullptr) override;
    bool stepX(qint32 relativePulses, const PrintAxisConfig& config,
        QString* errorMessage = nullptr) override;
    bool waitXStopped(int timeoutMs, const std::atomic_bool& cancelRequested,
        QString* errorMessage = nullptr) override;
    bool stopMappedAxes(QString* errorMessage = nullptr) override;
    bool waitMappedAxesStopped(int timeoutMs, QString* errorMessage = nullptr) override;
    bool returnToLogicalZero(const PrintAxisConfig& xConfig,
        const PrintAxisConfig& yConfig, int timeoutMs,
        QString* errorMessage = nullptr) override;
    bool verifyLogicalZero(QString* errorMessage = nullptr) override;

private:
    bool homeAxis(PrintHardwareProfile::LogicalAxis logicalAxis, QString* errorMessage);
    bool callSucceeded(int code, const char* functionName, short axis,
        QString* errorMessage) const;
    bool cleanupHardware(QString* errorMessage);
    bool cancellationRequested(short activeAxis, QString* errorMessage);
    short physicalAxis(PrintHardwareProfile::LogicalAxis logicalAxis) const;
    int homeDirection(PrintHardwareProfile::LogicalAxis logicalAxis) const;
    int homeBackoff(PrintHardwareProfile::LogicalAxis logicalAxis) const;
    bool acquireOwnership(QString* errorMessage);
    void releaseOwnership();
    void poisonOwnership(const QString& reason);
    bool startPrintMove(short axis, qint32 absoluteTarget,
        const PrintAxisConfig& config, QString* errorMessage);
    bool waitPrintAxisStopped(short axis, int timeoutMs,
        const std::atomic_bool* cancelRequested, QString* errorMessage);

    IImc60gApi* api_;
    PrintHardwareProfile profile_;
    std::unique_ptr<IImc60gClock> ownedClock_;
    IImc60gClock* clock_;
    mutable QMutex mutex_;
    std::atomic<bool> cancelRequested_ {false};
    Imc60gConnectionState state_ = Imc60gConnectionState::Disconnected;
    bool printActive_ = false;
    bool cardOpened_ = false;
    bool ethercatTouched_ = false;
    bool servoEnabled_[2] = {false, false};
};

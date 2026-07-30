#pragma once

#include "PrintConfig.h"
#include "PrintHardwareProfile.h"

#include <QMutex>
#include <QString>

class IImc60gApi;

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

class Imc60gMotionController {
public:
    Imc60gMotionController(IImc60gApi* api, const PrintHardwareProfile& profile);
    ~Imc60gMotionController();

    Imc60gConnectionState state() const;
    bool connectAndHome(QString* errorMessage = nullptr);
    void disconnect();

    bool moveRelative(PrintHardwareProfile::LogicalAxis logicalAxis, double millimeters,
        const PrintAxisConfig& axisConfig, QString* errorMessage = nullptr);
    bool stopAxis(PrintHardwareProfile::LogicalAxis logicalAxis,
        QString* errorMessage = nullptr);
    bool readSnapshot(PrintHardwareProfile::LogicalAxis logicalAxis,
        Imc60gAxisSnapshot* snapshot, QString* errorMessage = nullptr);
    bool isReadyForPrint() const;
    void setPrintActive(bool active);

private:
    bool homeAxis(PrintHardwareProfile::LogicalAxis logicalAxis, QString* errorMessage);
    bool callSucceeded(int code, const char* functionName, short axis,
        QString* errorMessage) const;
    void cleanupHardware();
    short physicalAxis(PrintHardwareProfile::LogicalAxis logicalAxis) const;
    int homeDirection(PrintHardwareProfile::LogicalAxis logicalAxis) const;
    int homeBackoff(PrintHardwareProfile::LogicalAxis logicalAxis) const;
    bool acquireOwnership(QString* errorMessage);
    void releaseOwnership();

    IImc60gApi* api_;
    PrintHardwareProfile profile_;
    mutable QMutex mutex_;
    Imc60gConnectionState state_ = Imc60gConnectionState::Disconnected;
    bool printActive_ = false;
    bool cardOpened_ = false;
    bool ethercatTouched_ = false;
    bool servoEnabled_[2] = {false, false};
};

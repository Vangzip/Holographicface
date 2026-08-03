#pragma once

#include "IMotionController.h"

#include <QLibrary>
#include <QString>

#include <array>

class DfjzhMotionController final : public IMotionController
{
public:
    explicit DfjzhMotionController(QString libraryPath = QString());
    ~DfjzhMotionController() override;

    bool initialize(QString* errorMessage = nullptr) override;
    void shutdown() override;

    bool setCurrentPositionAsOrigin(int axis, QString* errorMessage = nullptr) override;
    bool moveTo(
        int axis,
        long targetPulse,
        const PrintAxisConfig& config,
        QString* errorMessage = nullptr) override;
    bool stopAxis(int axis, QString* errorMessage = nullptr) override;
    bool waitUntilStopped(
        int axis,
        int timeoutMs,
        const std::atomic_bool& cancelRequested,
        QString* errorMessage = nullptr) override;
    long readPosition(int axis) const override;

    bool setExposureOutput(bool enabled, int outputIndex, QString* errorMessage = nullptr) override;
    bool armExposureWindow(long beginPos, long endPos, QString* errorMessage = nullptr) override;
    void disarmExposureWindow() override;

private:
    using InitCard = short(__stdcall*)(unsigned short, short, short, short, short, short);
    using ExitCard = short(__stdcall*)(unsigned short);
    using AxisHome = short(__stdcall*)(unsigned short, unsigned short);
    using AxisLong = short(__stdcall*)(unsigned short, unsigned short, long);
    using AxisStart = short(__stdcall*)(unsigned short, unsigned short);
    using AxisState = short(__stdcall*)(unsigned short, unsigned short);
    using AxisPosition = long(__stdcall*)(unsigned short, unsigned short);
    using WriteIoBit = short(__stdcall*)(unsigned short, unsigned short, short);
    using AxisIo = short(__stdcall*)(unsigned short, unsigned short, short, short, short, short);
    using AxisWindow = short(__stdcall*)(unsigned short, unsigned short, long, long);
    using EnableAxisWindow = short(__stdcall*)(unsigned short, unsigned short, short);

    bool loadLibrary(QString* errorMessage);
    bool resolveRequiredFunctions(QString* errorMessage);
    bool ensureInitialized(QString* errorMessage) const;
    bool validAxis(int axis, QString* errorMessage) const;
    void resetFunctions();

    QString libraryPath_;
    QLibrary library_;
    bool initialized_ = false;
    bool exposureWindowArmed_ = false;

    InitCard initCard_ = nullptr;
    ExitCard exitCard_ = nullptr;
    AxisHome home_ = nullptr;
    AxisLong setAxisAcc_ = nullptr;
    AxisLong setAxisDec_ = nullptr;
    AxisLong setAxisVel_ = nullptr;
    AxisLong setAxisStartVel_ = nullptr;
    AxisLong setAxisStopVel_ = nullptr;
    AxisLong setAxisStopDec_ = nullptr;
    AxisLong setAxisPos_ = nullptr;
    AxisStart startAxis_ = nullptr;
    AxisStart ceaseAxis_ = nullptr;
    AxisState readAxisState_ = nullptr;
    AxisPosition readAxisPos_ = nullptr;
    WriteIoBit writeIoBit_ = nullptr;
    AxisIo setAxisIo_ = nullptr;
    AxisWindow setIoPos_ = nullptr;
    EnableAxisWindow enableIoPos_ = nullptr;
};

#pragma once

#include "PrintConfig.h"
#include "PrintImageSource.h"

#include <QObject>
#include <QString>

enum class PrintUiState {
    Disconnected,
    Connecting,
    Homing,
    Ready,
    Printing,
    Paused,
    Stopping,
    Fault
};
Q_DECLARE_METATYPE(PrintUiState)

class IPrintController : public QObject
{
    Q_OBJECT

public:
    explicit IPrintController(QObject* parent = nullptr) : QObject(parent) {}
    ~IPrintController() override = default;

public slots:
    virtual void connectAndHome() = 0;
    virtual void disconnect() = 0;
    virtual void moveXNegative(double millimeters, const PrintAxisConfig& config) = 0;
    virtual void moveXPositive(double millimeters, const PrintAxisConfig& config) = 0;
    virtual void moveYNegative(double millimeters, const PrintAxisConfig& config) = 0;
    virtual void moveYPositive(double millimeters, const PrintAxisConfig& config) = 0;
    virtual void stopManualMotion() = 0;
    virtual void setLogicalOrigin() = 0;
    virtual void returnToLogicalOrigin() = 0;
    virtual void start(const Print9030Config& config, const PrintImageSet& images) = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void cancel() = 0;

signals:
    void stateChanged(PrintUiState state);
    void statusChanged(const QString& status);
    void errorChanged(const QString& detail);
    void progressChanged(int percent, const QString& detail);
    void positionsChanged(double xMillimeters, double yMillimeters);
    void hardwareStatusChanged(bool ethercat, bool servoX, bool servoY,
        bool homeX, bool homeY);
    void safeStopCompleted();
};

#ifndef PRINT9030_CONTROLLER_INTERFACE_ONLY
class PrintController final : public IPrintController
{
    Q_OBJECT

public:
    explicit PrintController(const QString& projectRoot, QObject* parent = nullptr);
    ~PrintController() override;

public slots:
    void connectAndHome() override;
    void disconnect() override;
    void moveXNegative(double millimeters, const PrintAxisConfig& config) override;
    void moveXPositive(double millimeters, const PrintAxisConfig& config) override;
    void moveYNegative(double millimeters, const PrintAxisConfig& config) override;
    void moveYPositive(double millimeters, const PrintAxisConfig& config) override;
    void stopManualMotion() override;
    void setLogicalOrigin() override;
    void returnToLogicalOrigin() override;
    void start(const Print9030Config& config, const PrintImageSet& images) override;
    void pause() override;
    void resume() override;
    void cancel() override;

private:
    class Worker;
    Worker* worker_ = nullptr;
    class QThread* workerThread_ = nullptr;
};
#endif

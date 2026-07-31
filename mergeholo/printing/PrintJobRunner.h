#pragma once

#include "PrintHardwarePreflight.h"

#include <QObject>

#include <atomic>

enum class PrintJobState {
    Ready,
    Printing,
    Paused,
    Stopping,
    Fault
};

class PrintJobRunner final : public QObject
{
    Q_OBJECT

public:
    PrintJobRunner(IMotionController& motion,
        IExposureController& exposure,
        IPrintFramePresenter& presenter,
        IPrintHardwarePreflight& preflight,
        QObject* parent = nullptr);
    ~PrintJobRunner() override;

    PrintJobState state() const;
    bool start(const PrintJobSnapshot& job, QString* errorMessage = nullptr);
    void requestPause();
    bool resume(QString* errorMessage = nullptr);
    void cancel();

signals:
    void stateChanged(PrintJobState state);
    void rowDirectionChanged(bool reverse);
    void frameAdvanced(int row, int column, int logicalFrame);
    void progressChanged(int value, const QString& text);
    void finished(bool success, const QString& text);

private:
    bool run(QString* errorMessage);
    bool failAndCleanup(const QString& primaryError, QString* errorMessage);
    bool finishAndCleanup(bool success, const QString& message,
        QString* errorMessage);
    bool cleanup(QString* errorMessage);
    bool operation(bool ok, const QString& fallback, const QString& detail,
        QString* errorMessage);
    void setState(PrintJobState state);

    IMotionController& motion_;
    IExposureController& exposure_;
    IPrintFramePresenter& presenter_;
    IPrintHardwarePreflight& preflight_;
    PrintJobSnapshot job_;
    QVector<PrintFrame> frames_;
    std::atomic_bool cancelRequested_ {false};
    std::atomic_bool pauseRequested_ {false};
    PrintJobState state_ = PrintJobState::Ready;
    int nextRow_ = 0;
    bool pendingXStep_ = false;
    bool printOwnership_ = false;
    bool presenterPrepared_ = false;
};

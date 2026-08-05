#pragma once

#include "PrintHardwarePreflight.h"
#include "PrintFlowLogger.h"
#include "V2RowDisplaySequence.h"

#include <QElapsedTimer>
#include <QObject>

#include <atomic>
#include <functional>
#include <memory>

enum class PrintJobState {
    Ready,
    Printing,
    Paused,
    Stopping,
    Fault
};

using V2DisplayThreadConfigurer = std::function<bool(QString*)>;

class PrintJobRunner final : public QObject
{
    Q_OBJECT

public:
    PrintJobRunner(IMotionController& motion,
        IExposureController& exposure,
        IPrintFramePresenter& presenter,
        IPrintHardwarePreflight& preflight,
        QObject* parent = nullptr);
    PrintJobRunner(IMotionController& motion,
        IExposureController& exposure,
        IPrintFramePresenter& presenter,
        IPrintHardwarePreflight& preflight,
        PrintImageQueueFactory imageQueueFactory,
        QObject* parent = nullptr);
    PrintJobRunner(IMotionController& motion,
        IExposureController& exposure,
        IPrintFramePresenter& presenter,
        IPrintHardwarePreflight& preflight,
        PrintImageQueueFactory imageQueueFactory,
        std::shared_ptr<IPrintFlowLogger> flowLogger,
        QObject* parent = nullptr);
    PrintJobRunner(IMotionController& motion,
        IExposureController& exposure,
        IPrintFramePresenter& presenter,
        IPrintHardwarePreflight& preflight,
        PrintImageQueueFactory imageQueueFactory,
        std::shared_ptr<IPrintFlowLogger> flowLogger,
        V2DisplayThreadConfigurer displayThreadConfigurer,
        QObject* parent = nullptr);
    ~PrintJobRunner() override;

    PrintJobState state() const;
    bool start(const PrintJobSnapshot& job, QString* errorMessage = nullptr);
    // Thread-safe request-only controls. They never call an SDK adapter.
    void requestPause();
    bool resume(QString* errorMessage = nullptr);
    void cancel();
    // Must run on this object's owner thread. It performs paused-state cleanup
    // after a thread-safe cancel request; active runs drain the request inline.
    bool processPendingControl(QString* errorMessage = nullptr);

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
    bool startImageQueue(QString* errorMessage);
    void stopImageQueue();
    bool prepareRow(const V2RowPlan& row, QVector<PrintFrame>* rowFrames,
        QVector<QSize>* targetSizes, QString* errorMessage);
    bool runV2DisplaySequence(const V2RowPlan& row,
        const QVector<PrintFrame>& rowFrames,
        const QVector<QSize>& targetSizes,
        const QVector<V2RefreshSubmission>& submissions,
        const std::function<bool(QString*)>& onRowPrepared,
        const std::function<bool(qint64 anchorQpcUs, QString*)>& onDisplayReady,
        QString* errorMessage);
    void setState(PrintJobState state);

    IMotionController& motion_;
    IExposureController& exposure_;
    IPrintFramePresenter& presenter_;
    IPrintHardwarePreflight& preflight_;
    PrintImageQueueFactory imageQueueFactory_;
    std::unique_ptr<IPrintImageQueue> imageQueue_;
    std::shared_ptr<IPrintFlowLogger> flowLogger_;
    V2DisplayThreadConfigurer displayThreadConfigurer_;
    PrintJobSnapshot job_;
    QElapsedTimer runClock_;
    std::atomic_bool cancelRequested_ {false};
    std::atomic_bool pauseRequested_ {false};
    std::atomic<PrintJobState> state_ {PrintJobState::Ready};
    int nextRow_ = 0;
    bool pendingXStep_ = false;
    bool printOwnership_ = false;
    bool presenterPrepared_ = false;
    bool presenterWarmupDone_ = false;
    int activeFlowRow_ = -1;
    bool flowMotionActive_ = false;
    bool flowExposureActive_ = false;
};

#include "PrintJobRunner.h"

#include <QtMath>
#include <QThread>

#include <cmath>
#include <condition_variable>
#include <exception>
#include <limits>
#include <mutex>
#include <thread>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {

void appendError(QString* destination, const QString& text)
{
    if (text.isEmpty()) return;
    if (!destination->isEmpty()) *destination += "; ";
    *destination += text;
}

QSize targetSize(const PrintFrame& frame, const Print9030Config& config,
    QString* errorMessage)
{
    const double widthScale = config.main.widthScale;
    const double heightScale = config.main.heightScale;
    const double widthValue = static_cast<double>(frame.width) * widthScale;
    const double heightValue = static_cast<double>(frame.height) * heightScale;
    if (!frame.isValid()
        || !std::isfinite(widthScale) || !std::isfinite(heightScale)
        || widthScale <= 0.0 || heightScale <= 0.0
        || !std::isfinite(widthValue) || !std::isfinite(heightValue)
        || widthValue <= 0.0 || heightValue <= 0.0
        || widthValue > std::numeric_limits<int>::max()
        || heightValue > std::numeric_limits<int>::max()) {
        if (errorMessage) *errorMessage = "Print frame or second-screen scale is invalid.";
        return {};
    }
    const qint64 width = static_cast<qint64>(widthValue);
    const qint64 height = static_cast<qint64>(heightValue);
    if (width <= 0 || height <= 0
        || width > std::numeric_limits<int>::max()
        || height > std::numeric_limits<int>::max()) {
        if (errorMessage) *errorMessage = "Print frame or second-screen scale is invalid.";
        return {};
    }
    return QSize(static_cast<int>(width), static_cast<int>(height));
}

int boundedTimeoutMs(const PrintJobSnapshot& job)
{
    const qint64 speed = job.config.axisY.speedOfMovement;
    if (speed <= 0) return 5000;
    const qint64 travelMs = job.plan.totalPulse * 1000 / speed;
    return static_cast<int>(qBound<qint64>(5000, travelMs * 2 + 5000, 300000));
}

QString directionName(bool reverse)
{
    return reverse ? QStringLiteral("reverse") : QStringLiteral("forward");
}

QString quotedFlowValue(QString value)
{
    value.replace('\\', QStringLiteral("\\\\"));
    value.replace('"', QStringLiteral("\\\""));
    value.replace('\r', QStringLiteral("\\r"));
    value.replace('\n', QStringLiteral("\\n"));
    return '"' + value + '"';
}

bool configureV2DisplayThread(QString* errorMessage)
{
#ifdef Q_OS_WIN
    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Cannot set V2 display worker to THREAD_PRIORITY_HIGHEST (Win32 error %1).")
                .arg(GetLastError());
        }
        return false;
    }
#endif
    if (errorMessage) errorMessage->clear();
    return true;
}

void v2SleepZero()
{
#ifdef Q_OS_WIN
    Sleep(0);
#else
    std::this_thread::yield();
#endif
}

} // namespace

PrintJobRunner::PrintJobRunner(IMotionController& motion,
    IExposureController& exposure, IPrintFramePresenter& presenter,
    IPrintHardwarePreflight& preflight, QObject* parent)
    : PrintJobRunner(motion, exposure, presenter, preflight,
        PrintImageQueueFactory(), {}, parent)
{
}

PrintJobRunner::PrintJobRunner(IMotionController& motion,
    IExposureController& exposure, IPrintFramePresenter& presenter,
    IPrintHardwarePreflight& preflight,
    PrintImageQueueFactory imageQueueFactory, QObject* parent)
    : PrintJobRunner(motion, exposure, presenter, preflight,
        std::move(imageQueueFactory), {}, parent)
{
}

PrintJobRunner::PrintJobRunner(IMotionController& motion,
    IExposureController& exposure, IPrintFramePresenter& presenter,
    IPrintHardwarePreflight& preflight,
    PrintImageQueueFactory imageQueueFactory,
    std::shared_ptr<IPrintFlowLogger> flowLogger, QObject* parent)
    : PrintJobRunner(motion, exposure, presenter, preflight,
        std::move(imageQueueFactory), std::move(flowLogger),
        configureV2DisplayThread, parent)
{
}

PrintJobRunner::PrintJobRunner(IMotionController& motion,
    IExposureController& exposure, IPrintFramePresenter& presenter,
    IPrintHardwarePreflight& preflight,
    PrintImageQueueFactory imageQueueFactory,
    std::shared_ptr<IPrintFlowLogger> flowLogger,
    V2DisplayThreadConfigurer displayThreadConfigurer, QObject* parent)
    : QObject(parent)
    , motion_(motion)
    , exposure_(exposure)
    , presenter_(presenter)
    , preflight_(preflight)
    , imageQueueFactory_(std::move(imageQueueFactory))
    , flowLogger_(std::move(flowLogger))
    , displayThreadConfigurer_(std::move(displayThreadConfigurer))
{
}

PrintJobRunner::~PrintJobRunner()
{
    const PrintJobState current = state();
    if (current == PrintJobState::Printing || current == PrintJobState::Paused
        || current == PrintJobState::Stopping) {
        cancelRequested_.store(true);
        stopImageQueue();
        QString ignored;
        cleanup(&ignored);
        state_.store(PrintJobState::Fault);
    } else {
        stopImageQueue();
    }
}

PrintJobState PrintJobRunner::state() const
{
    return state_.load(std::memory_order_acquire);
}

void PrintJobRunner::setState(PrintJobState state)
{
    state_.store(state, std::memory_order_release);
    emit stateChanged(state);
}

bool PrintJobRunner::start(const PrintJobSnapshot& job, QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
    if (state() != PrintJobState::Ready) {
        if (errorMessage) *errorMessage = "Print job is not Ready for a new start.";
        return false;
    }
    const PrintPreflightResult preflight = preflight_.check(job, false);
    if (!preflight.ok) {
        setState(PrintJobState::Fault);
        if (errorMessage) *errorMessage = preflight.detail;
        emit finished(false, preflight.detail);
        return false;
    }

    job_ = job;
    QString detail;
    if (!startImageQueue(&detail)) {
        stopImageQueue();
        setState(PrintJobState::Fault);
        if (errorMessage) *errorMessage = detail;
        emit finished(false, detail);
        return false;
    }
    if (!motion_.beginPrint(&detail)) {
        stopImageQueue();
        setState(PrintJobState::Fault);
        if (errorMessage) *errorMessage = detail.isEmpty() ? "Cannot acquire IMC60G print ownership." : detail;
        emit finished(false, errorMessage ? *errorMessage : detail);
        return false;
    }
    printOwnership_ = true;
    cancelRequested_.store(false);
    pauseRequested_.store(false);
    nextRow_ = 0;
    pendingXStep_ = false;
    presenterPrepared_ = false;
    presenterWarmupDone_ = false;
    activeFlowRow_ = -1;
    flowMotionActive_ = false;
    flowExposureActive_ = false;
    runClock_.start();
    if (flowLogger_) {
        flowLogger_->log(QString(
            "event=run_start total_rows=%1 total_columns=%2 frames_per_image=%3")
            .arg(job_.plan.rows.size())
            .arg(job_.config.main.gridColumns)
            .arg(job_.plan.framesPerImage));
    }
    setState(PrintJobState::Printing);
    return run(errorMessage);
}

void PrintJobRunner::requestPause()
{
    pauseRequested_.store(true, std::memory_order_release);
}

bool PrintJobRunner::resume(QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
    if (state() != PrintJobState::Paused) {
        if (errorMessage) *errorMessage = "Only a Paused job can resume.";
        return false;
    }
    if (cancelRequested_.load(std::memory_order_acquire)) {
        return processPendingControl(errorMessage);
    }
    const PrintPreflightResult dynamic = preflight_.check(job_, true);
    if (!dynamic.ok) return failAndCleanup("Dynamic resume preflight failed: " + dynamic.detail, errorMessage);
    QString detail;
    if (!startImageQueue(&detail))
        return failAndCleanup(detail, errorMessage);
    pauseRequested_.store(false);
    setState(PrintJobState::Printing);
    return run(errorMessage);
}

void PrintJobRunner::cancel()
{
    cancelRequested_.store(true, std::memory_order_release);
}

bool PrintJobRunner::processPendingControl(QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
    if (QThread::currentThread() != thread()) {
        if (errorMessage) {
            *errorMessage =
                "Print control cleanup must run on the runner owner thread.";
        }
        return false;
    }
    if (state() == PrintJobState::Paused
        && cancelRequested_.load(std::memory_order_acquire)) {
        return finishAndCleanup(
            false, "Print job was cancelled.", errorMessage);
    }
    return true;
}

bool PrintJobRunner::operation(bool ok, const QString& fallback,
    const QString& detail, QString* errorMessage)
{
    if (ok) return true;
    if (errorMessage) *errorMessage = detail.isEmpty() ? fallback : detail;
    return false;
}

bool PrintJobRunner::startImageQueue(QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
    if (!imageQueue_) {
        imageQueue_ = imageQueueFactory_
            ? imageQueueFactory_(job_.images)
            : job_.images.createQueue();
    }
    if (!imageQueue_) {
        if (errorMessage)
            *errorMessage = "Cannot create the V2 print image prefetch queue.";
        return false;
    }
    return imageQueue_->start(errorMessage);
}

void PrintJobRunner::stopImageQueue()
{
    if (!imageQueue_) return;
    imageQueue_->stop();
    imageQueue_.reset();
}

bool PrintJobRunner::prepareRow(const V2RowPlan& row,
    QVector<PrintFrame>* rowFrames, QVector<QSize>* targetSizes,
    QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
    if (!rowFrames || !targetSizes || row.logicalFrameOrder.isEmpty()) {
        if (errorMessage)
            *errorMessage = "V2 row preparation requires complete output buffers and frame order.";
        return false;
    }

    QString detail;
    if (!imageQueue_
        || !imageQueue_->takeRow(job_.config.main.gridColumns,
            rowFrames, &detail)) {
        if (errorMessage) {
            *errorMessage = detail.isEmpty()
                ? "Cannot obtain a complete prefetched print row." : detail;
        }
        return false;
    }
    if (rowFrames->size() != job_.config.main.gridColumns) {
        if (errorMessage)
            *errorMessage = "Prefetched print row does not match the planned column count.";
        return false;
    }

    const QSize rowTargetSize = targetSize(rowFrames->first(), job_.config, &detail);
    if (rowTargetSize.isEmpty()) {
        if (errorMessage) *errorMessage = detail;
        return false;
    }
    targetSizes->fill(rowTargetSize, rowFrames->size());

    const int anchorLogicalFrame = row.logicalFrameOrder.first();
    if (anchorLogicalFrame < 0 || anchorLogicalFrame >= rowFrames->size()) {
        if (errorMessage)
            *errorMessage = "V2 row anchor frame is outside the prepared source row.";
        return false;
    }

    if (!presenterPrepared_) {
        const bool initialized = presenter_.prepare(
            rowFrames->at(anchorLogicalFrame),
            targetSizes->at(anchorLogicalFrame), &detail);
        if (!initialized) {
            if (errorMessage) {
                *errorMessage = detail.isEmpty()
                    ? "Initial prepared-row presenter warm-up failed." : detail;
            }
            return false;
        }
        presenterPrepared_ = true;
    }

    return true;
}

bool PrintJobRunner::runV2DisplaySequence(const V2RowPlan& row,
    const QVector<PrintFrame>& rowFrames,
    const QVector<QSize>& targetSizes,
    const QVector<V2RefreshSubmission>& submissions,
    const std::function<bool(QString*)>& onRowPrepared,
    const std::function<bool(qint64 anchorQpcUs, QString*)>& onDisplayReady,
    QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
    if (row.logicalFrameOrder.isEmpty()
        || rowFrames.isEmpty() || rowFrames.size() != targetSizes.size()
        || submissions.isEmpty() || !onRowPrepared || !onDisplayReady) {
        if (errorMessage)
            *errorMessage = "V2 display sequence requires a complete row, refresh submissions, and readiness callbacks.";
        return false;
    }

    std::atomic_bool schedulerFailed {false};
    std::atomic_bool displayWorkerStop {false};
    std::atomic_int presentedColumn {-1};
    std::atomic_int rowDisplayRequestedPic {-1};
    std::atomic_int rowDisplayRequestedCol {-1};
    std::atomic_int rowDisplayRequestSeq {0};
    QString schedulerError;
    std::mutex gateMutex;
    std::condition_variable gateChanged;
    bool continueToAnchor = false;
    bool displayReady = false;
    bool displayPollingEnabled = false;
    bool sequenceStarted = false;
    bool sequenceDone = false;
    PrintRowVBlankAnchor anchor;
    qint64 primeBeginUs = 0;
    qint64 primeEndUs = 0;
    qint64 anchorWaitBeginUs = 0;
    qint64 anchorUs = 0;
    std::thread displayWorker;
    const auto stopAndJoinDisplayWorker = [&] {
        displayWorkerStop.store(true, std::memory_order_release);
        gateChanged.notify_all();
        if (displayWorker.joinable()) displayWorker.join();
    };
    try {
        displayWorker = std::thread([&] {
            const auto publishFailure = [&](const QString& fallback,
                                            const QString& detail) {
                presenter_.clearPreparedRow();
                schedulerError = detail.isEmpty() ? fallback : detail;
                schedulerFailed.store(true, std::memory_order_release);
                gateChanged.notify_all();
            };
            QString detail;
            if (!displayThreadConfigurer_
                || !displayThreadConfigurer_(&detail)) {
                publishFailure(
                    "Cannot configure the strict V2 display worker.", detail);
                return;
            }
            {
                std::unique_lock<std::mutex> gateLock(gateMutex);
                gateChanged.wait(gateLock, [&] {
                    return continueToAnchor || displayWorkerStop.load(std::memory_order_acquire);
                });
                if (displayWorkerStop.load(std::memory_order_acquire)) {
                    presenter_.clearPreparedRow();
                    return;
                }
            }

            primeBeginUs = runClock_.nsecsElapsed() / 1000;
            const int anchorLogicalFrame = row.logicalFrameOrder.first();
            if (!presenter_.presentPreparedRowFrame(anchorLogicalFrame, &detail)) {
                publishFailure("Prepared row prime Present failed.", detail);
                return;
            }
            primeEndUs = runClock_.nsecsElapsed() / 1000;
            anchorWaitBeginUs = primeEndUs;
            if (!presenter_.acquireRowAnchor(&anchor, &detail)) {
                publishFailure("Physical row VBlank anchor failed.", detail);
                return;
            }
            anchorUs = runClock_.nsecsElapsed() / 1000;
            {
                std::lock_guard<std::mutex> gateLock(gateMutex);
                displayReady = true;
            }
            gateChanged.notify_all();

            // V2 creates the high-priority row display loop only once Y has
            // reached the constant segment. It must not spin during Y
            // acceleration after the row anchor is acquired.
            {
                std::unique_lock<std::mutex> gateLock(gateMutex);
                gateChanged.wait(gateLock, [&] {
                    return displayPollingEnabled
                        || displayWorkerStop.load(std::memory_order_acquire);
                });
                if (displayWorkerStop.load(std::memory_order_acquire)) {
                    presenter_.clearPreparedRow();
                    return;
                }
            }

            // V2's display worker is high priority and polls request sequence
            // directly.  A condition-variable wakeup here can miss the anchor
            // phase and delay the first queued image by a refresh.
            int lastDisplayedRequestSeq = 0;
            for (;;) {
                if (displayWorkerStop.load(std::memory_order_acquire)) {
                    presenter_.clearPreparedRow();
                    return;
                }
                const int currentRequestSeq = rowDisplayRequestSeq.load(std::memory_order_acquire);
                if (currentRequestSeq == lastDisplayedRequestSeq) {
                    v2SleepZero();
                    continue;
                }
                const int requestedPic = rowDisplayRequestedPic.load(std::memory_order_acquire);
                const int requestedCol = rowDisplayRequestedCol.load(std::memory_order_acquire);
                if (rowDisplayRequestSeq.load(std::memory_order_acquire) != currentRequestSeq) {
                    v2SleepZero();
                    continue;
                }
                if (requestedPic < 0 || requestedPic >= row.logicalFrameOrder.size()
                    || requestedCol != 0) {
                    v2SleepZero();
                    continue;
                }
                lastDisplayedRequestSeq = currentRequestSeq;
                {
                    std::lock_guard<std::mutex> gateLock(gateMutex);
                    sequenceStarted = true;
                }
                gateChanged.notify_all();
                break;
            }

            // After consuming the one V2 request, the display worker owns only
            // the physical-VBlank/Present sequence and never reads motion.
            const int framesPerImage = row.holdFramesAfterPresent + 1;
            int lastVisibleColumn = -1;
            for (const V2RefreshSubmission& submission : submissions) {
                if (displayWorkerStop.load(std::memory_order_acquire)) {
                    presenter_.clearPreparedRow();
                    return;
                }
                if (cancelRequested_.load(std::memory_order_acquire)) {
                    publishFailure("Print job was cancelled.", {});
                    return;
                }
                detail.clear();
                const int rowSlot = submission.usesRowAnchor ? 0 : submission.refreshSlot;
                const qint64 frameWaitBeginUs = runClock_.nsecsElapsed() / 1000;
                if (!presenter_.waitForRowSlot(rowSlot, &anchor, &detail)) {
                    publishFailure("Print-frame physical VBlank failed.", detail);
                    return;
                }
                const qint64 frameWaitEndUs = runClock_.nsecsElapsed() / 1000;
                // V2 queries the previous presentation's frame statistics at
                // this exact point: after the slot VBlank and before Present.
                // The query is deliberately telemetry-only and cannot change
                // the success path of the next paced submission.
                presenter_.sampleFrameStatistics();
                const PrintRowVBlankCounterSnapshot counterBeforePresent =
                    presenter_.rowVBlankCounterSnapshot();
                const qint64 presentBeginUs = runClock_.nsecsElapsed() / 1000;
                if (!presenter_.presentPreparedRowFrame(
                        submission.submittedLogicalFrame, &detail)) {
                    publishFailure("Prepared print-frame Present failed.", detail);
                    return;
                }
                const qint64 presentEndUs = runClock_.nsecsElapsed() / 1000;
                const PrintRowVBlankCounterSnapshot counterAfterPresent =
                    presenter_.rowVBlankCounterSnapshot();
                // This query is deliberately after frame_after_present, exactly
                // where V2 records DXGI_FRAME_STATISTICS_MEDIA. It is logging
                // only and never participates in a VBlank-paced submission.
                const PrintFrameStatisticsMedia mediaStatistics =
                    presenter_.sampleFrameStatisticsMedia();
                const qint64 estimatedPresentQpcUs = anchor.qpcUs > 0
                    ? anchor.qpcUs + (presentBeginUs - anchorUs) : -1;
                const qint64 requestOffsetUs = estimatedPresentQpcUs >= 0
                        && counterBeforePresent.lastVBlankQpcUs > 0
                    ? estimatedPresentQpcUs - counterBeforePresent.lastVBlankQpcUs : -1;
                if (flowLogger_) {
                    flowLogger_->log(QString(
                        "event=dxgi_present_submit row=%1 dir=%2 refresh_slot=%3 "
                        "visible_col=%4 slot_in_image=%5 visible_logical_pic=%6 "
                        "submit_col=%7 submit_logical_pic=%8 frame_wait_result=0 "
                        "frame_wait_cost_us=%9 vblank_method=%10 frame_before_present=%11 "
                        "frame_after_present=%12 request_offset_us=%13 present_cost_us=%14 "
                        "media_stats_hr=0x%15 media_present_count=%16 "
                        "media_present_refresh_count=%17 media_sync_refresh_count=%18 "
                        "media_sync_qpc_us=%19 composition_mode=%20 "
                        "row_anchor_source=swap_chain_output_vblank")
                        .arg(row.row + 1).arg(directionName(row.reverse))
                        .arg(submission.refreshSlot).arg(submission.visibleColumn)
                        .arg(submission.refreshSlot % framesPerImage)
                        .arg(submission.visibleLogicalFrame).arg(submission.submittedColumn)
                        .arg(submission.submittedLogicalFrame)
                        .arg(frameWaitEndUs - frameWaitBeginUs)
                        .arg(submission.usesRowAnchor ? QStringLiteral("row_prime")
                                                        : QStringLiteral("monitor_output"))
                        .arg(counterBeforePresent.frame).arg(counterAfterPresent.frame)
                        .arg(requestOffsetUs).arg(presentEndUs - presentBeginUs)
                        .arg(static_cast<quint32>(mediaStatistics.hresult), 8, 16,
                            QLatin1Char('0'))
                        .arg(mediaStatistics.presentCount)
                        .arg(mediaStatistics.presentRefreshCount)
                        .arg(mediaStatistics.syncRefreshCount)
                        .arg(mediaStatistics.syncQpcUs)
                        .arg(mediaStatistics.compositionMode));
                }
                if (submission.visibleColumn != lastVisibleColumn) {
                    lastVisibleColumn = submission.visibleColumn;
                    presentedColumn.store(lastVisibleColumn, std::memory_order_release);
                    emit frameAdvanced(row.row, submission.visibleColumn,
                        submission.visibleLogicalFrame);
                }
            }
            presenter_.clearPreparedRow();
            {
                std::lock_guard<std::mutex> gateLock(gateMutex);
                sequenceDone = true;
            }
            gateChanged.notify_all();
        });
    } catch (const std::exception& exception) {
        if (errorMessage) {
            *errorMessage = QString("Cannot start V2 display worker: %1")
                .arg(QString::fromLocal8Bit(exception.what()));
        }
        return false;
    }

    QString preparedError;
    const bool prepared = onRowPrepared(&preparedError);
    {
        std::lock_guard<std::mutex> gateLock(gateMutex);
        continueToAnchor = prepared;
    }
    if (!prepared) {
        displayWorkerStop.store(true, std::memory_order_release);
    }
    gateChanged.notify_all();
    if (!prepared) {
        displayWorker.join();
        if (errorMessage) {
            *errorMessage = preparedError.isEmpty()
                ? "V2 row preparation did not enter VBlank anchoring." : preparedError;
        }
        return false;
    }

    {
        std::unique_lock<std::mutex> gateLock(gateMutex);
        gateChanged.wait(gateLock, [&] {
            return displayReady || schedulerFailed.load(std::memory_order_acquire);
        });
    }
    if (schedulerFailed.load(std::memory_order_acquire)) {
        displayWorker.join();
        if (errorMessage) *errorMessage = schedulerError;
        return false;
    }

    const int anchorLogicalFrame = row.logicalFrameOrder.first();
    if (flowLogger_) {
        flowLogger_->log(QString(
            "event=dxgi_row_anchor row=%1 dir=%2 prime_logical_pic=%3 "
            "prime_begin_qpc_us=%4 prime_end_qpc_us=%5 prime_cost_us=%6 "
            "wait_result=0 wait_cost_us=%7 anchor_qpc_us=%8 ready=%9 "
            "presenter_generation=%10 source=swap_chain_output_vblank "
            "directflip_candidate=1")
            .arg(row.row + 1)
            .arg(directionName(row.reverse))
            .arg(anchorLogicalFrame)
            .arg(primeBeginUs)
            .arg(primeEndUs)
            .arg(primeEndUs - primeBeginUs)
            .arg(anchorUs - anchorWaitBeginUs)
            .arg(anchorUs)
            .arg(anchor.available ? 1 : 0)
            .arg(anchor.presenterGeneration));
    }

    QString readyError;
    presenter_.beginRowVBlankCounter(anchor.qpcUs);
    const bool ready = onDisplayReady(anchorUs, &readyError);
    if (!ready) {
        displayWorkerStop.store(true, std::memory_order_release);
        gateChanged.notify_all();
        if (displayWorker.joinable()) displayWorker.join();
        if (errorMessage) {
            *errorMessage = readyError.isEmpty()
                ? "V2 display sequence did not enter motion." : readyError;
        }
        return false;
    }

    // This is the V2 print-thread selector.  It owns axis reads and posts the
    // first request; the display worker only consumes that request and then
    // performs its physical-VBlank/Present sequence.
    const qint64 constantEndPosition = row.yTarget
        + (row.reverse ? job_.plan.accelerationPulse : -job_.plan.accelerationPulse);
    const auto hasReachedConstantBegin = [&](qint64 yPosition) {
        return row.reverse ? yPosition <= row.constantBegin
                           : yPosition >= row.constantBegin;
    };
    const auto hasReachedConstantEnd = [&](qint64 yPosition) {
        return row.reverse ? yPosition <= constantEndPosition
                           : yPosition >= constantEndPosition;
    };
    const auto failAfterStoppingWorker = [&](const QString& message) {
        stopAndJoinDisplayWorker();
        if (errorMessage) *errorMessage = message;
        return false;
    };
    const auto sampleYPosition = [&](qint32* yPosition, QString* sampleError) {
        return motion_.readMappedYPlannedPosition(yPosition, sampleError);
    };

    // Match V2's acceleration loop: it reads Y once per millisecond until the
    // constant segment begins, and only then starts the display request loop.
    for (;;) {
        if (schedulerFailed.load(std::memory_order_acquire))
            return failAfterStoppingWorker(schedulerError);
        if (cancelRequested_.load(std::memory_order_acquire))
            return failAfterStoppingWorker("Print job was cancelled.");
        qint32 currentYPosition = 0;
        QString selectorError;
        if (!sampleYPosition(&currentYPosition, &selectorError))
            return failAfterStoppingWorker(selectorError);
        if (hasReachedConstantBegin(currentYPosition)) break;
#ifdef Q_OS_WIN
        Sleep(1);
#else
        QThread::msleep(1);
#endif
    }
    {
        std::lock_guard<std::mutex> gateLock(gateMutex);
        displayPollingEnabled = true;
    }
    gateChanged.notify_all();

    qint32 firstGateObservedY = 0;
    qint64 firstGateSampleUs = 0;
    const qint64 firstImageTargetPosition = row.constantBegin
        + (row.reverse ? -row.effectiveDisplayDelayPulse
                       : row.effectiveDisplayDelayPulse);
    const qint64 firstImageGatePosition = firstImageTargetPosition
        + (row.reverse ? job_.plan.presentPredictPulse
                       : -job_.plan.presentPredictPulse);
    bool reachedConstantEnd = false;
    for (;;) {
        if (schedulerFailed.load(std::memory_order_acquire))
            return failAfterStoppingWorker(schedulerError);
        if (cancelRequested_.load(std::memory_order_acquire))
            return failAfterStoppingWorker("Print job was cancelled.");

        QString selectorError;
        if (!sampleYPosition(&firstGateObservedY, &selectorError))
            return failAfterStoppingWorker(selectorError);
        if (hasReachedConstantEnd(firstGateObservedY)) {
            reachedConstantEnd = true;
            break;
        }

        qint64 selectedYPosition = firstGateObservedY;
        if (job_.plan.presentPredictPulse > 0) {
            selectedYPosition += row.reverse ? -job_.plan.presentPredictPulse
                                             : job_.plan.presentPredictPulse;
        }
        const qint64 movedPulse = row.reverse
            ? row.constantBegin - selectedYPosition
            : selectedYPosition - row.constantBegin;
        const qint64 adjustedPulse = movedPulse - row.effectiveDisplayDelayPulse;
        if (adjustedPulse < 0) {
            v2SleepZero();
            continue;
        }
        qint64 selectedColumn = job_.plan.stepPulse > 0
            ? (adjustedPulse + job_.plan.stepPulse / 2) / job_.plan.stepPulse : 0;
        selectedColumn = qBound<qint64>(0, selectedColumn,
            row.logicalFrameOrder.size() - 1);

        // V2's verified sequence branch uses the Y gate only to request its
        // first image. The display worker owns every later refresh slot.
        selectedColumn = 0;
        firstGateSampleUs = runClock_.nsecsElapsed() / 1000;
        const int logicalPic = row.logicalFrameOrder.at(static_cast<int>(selectedColumn));
        rowDisplayRequestedPic.store(logicalPic, std::memory_order_release);
        rowDisplayRequestedCol.store(static_cast<int>(selectedColumn), std::memory_order_release);
        const int requestSeq = rowDisplayRequestSeq.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (flowLogger_) {
            flowLogger_->log(QString(
                "event=v2_sequence_position_gate row=%1 dir=%2 "
                "constant_begin_pulse=%3 effective_display_delay_pulse=%4 "
                "first_image_target_pulse=%5 present_predict_pulse=%6 "
                "first_image_gate_pulse=%7 observed_y_planned_pulse=%8 "
                "gate_qpc_us=%9")
                .arg(row.row + 1).arg(directionName(row.reverse))
                .arg(row.constantBegin).arg(row.effectiveDisplayDelayPulse)
                .arg(firstImageTargetPosition).arg(job_.plan.presentPredictPulse)
                .arg(firstImageGatePosition).arg(firstGateObservedY)
                .arg(firstGateSampleUs));
            flowLogger_->log(QString(
                "event=first_column_request row=%1 dir=%2 col=0 logical_pic=%3 "
                "request_seq=%4 y_before=%5 target_y=%6")
                .arg(row.row + 1).arg(directionName(row.reverse))
                .arg(logicalPic).arg(requestSeq)
                .arg(firstGateObservedY).arg(firstImageTargetPosition));
        }
        {
            std::unique_lock<std::mutex> gateLock(gateMutex);
            gateChanged.wait(gateLock, [&] {
                return sequenceStarted || schedulerFailed.load(std::memory_order_acquire);
            });
        }
        break;
    }

    int lastMonitoredColumn = -1;
    const auto monitorPresentedColumn = [&] {
        const int currentColumn = presentedColumn.load(std::memory_order_acquire);
        if (currentColumn <= lastMonitoredColumn) return;
        lastMonitoredColumn = currentColumn;
        const qint64 sampleBeginUs = runClock_.nsecsElapsed() / 1000;
        qint32 yPosition = 0;
        QString telemetryError;
        const bool telemetryAvailable = motion_.readMappedYPlannedPosition(
            &yPosition, &telemetryError);
        const qint64 sampleEndUs = runClock_.nsecsElapsed() / 1000;
        const int visibleLogicalFrame = row.reverse
            ? row.logicalFrameOrder.size() - 1 - currentColumn : currentColumn;
        const int submittedColumn = qMin(currentColumn + 1,
            row.logicalFrameOrder.size() - 1);
        const int submittedLogicalFrame = row.reverse
            ? row.logicalFrameOrder.size() - 1 - submittedColumn : submittedColumn;
        if (flowLogger_) {
            flowLogger_->log(QString(
                "event=dxgi_present_axis_relation row=%1 dir=%2 refresh_slot=%3 "
                "visible_col=%4 visible_logical_pic=%5 submit_logical_pic=%6 "
                "sample_begin_qpc_us=%7 sample_end_qpc_us=%8 axis_sample_available=%9 "
                "x_planned_pulse=%10 x_encoder_pulse=%11 y_planned_pulse=%12 "
                "y_encoder_pulse=%13")
                .arg(row.row + 1).arg(directionName(row.reverse))
                .arg(currentColumn * (row.holdFramesAfterPresent + 1))
                .arg(currentColumn).arg(visibleLogicalFrame).arg(submittedLogicalFrame)
                .arg(sampleBeginUs).arg(sampleEndUs).arg(telemetryAvailable ? 1 : 0)
                .arg(0).arg(0).arg(yPosition).arg(0));
        }
    };

    const auto isSequenceDone = [&] {
        std::lock_guard<std::mutex> gateLock(gateMutex);
        return sequenceDone;
    };
    // V2 monitors the displayed column while the display worker submits the
    // consecutive sequence.  Completion of that sequence does not end the
    // row: the print thread still waits for the physical constant-end point.
    while (!reachedConstantEnd && !isSequenceDone()) {
        if (schedulerFailed.load(std::memory_order_acquire))
            return failAfterStoppingWorker(schedulerError);
        if (cancelRequested_.load(std::memory_order_acquire))
            return failAfterStoppingWorker("Print job was cancelled.");

        monitorPresentedColumn();
        qint32 currentYPosition = 0;
        QString selectorError;
        if (!sampleYPosition(&currentYPosition, &selectorError))
            return failAfterStoppingWorker(selectorError);
        if (hasReachedConstantEnd(currentYPosition)) {
            reachedConstantEnd = true;
            break;
        }
        v2SleepZero();
    }
    monitorPresentedColumn();

    while (!reachedConstantEnd) {
        if (schedulerFailed.load(std::memory_order_acquire))
            return failAfterStoppingWorker(schedulerError);
        if (cancelRequested_.load(std::memory_order_acquire))
            return failAfterStoppingWorker("Print job was cancelled.");

        qint32 currentYPosition = 0;
        QString selectorError;
        if (!sampleYPosition(&currentYPosition, &selectorError))
            return failAfterStoppingWorker(selectorError);
        if (hasReachedConstantEnd(currentYPosition)) {
            reachedConstantEnd = true;
            break;
        }
        v2SleepZero();
    }

    // V2 reaches the physical constant-segment end before it stops the row
    // display thread and RowVBlankCounter.  Do not stop either at submission
    // 99, otherwise exposure is closed early.
    stopAndJoinDisplayWorker();
    if (schedulerFailed.load(std::memory_order_acquire)) {
        if (errorMessage) *errorMessage = schedulerError;
        return false;
    }
    if (cancelRequested_.load(std::memory_order_acquire)) {
        if (errorMessage) *errorMessage = "Print job was cancelled.";
        return false;
    }
    return true;
}

bool PrintJobRunner::run(QString* errorMessage)
{
    const int timeoutMs = boundedTimeoutMs(job_);
    while (nextRow_ < job_.plan.rows.size()) {
        if (cancelRequested_.load()) return failAndCleanup("Print job was cancelled.", errorMessage);

        if (pendingXStep_) {
            const qint64 pulsesPerMillimeter = static_cast<qint64>(job_.config.axisX.subdivision)
                * static_cast<qint64>(job_.config.axisX.resolution);
            qint64 step = qRound64(job_.config.main.rowSpacingMm * pulsesPerMillimeter);
            if (job_.config.axisX.changeDirection) step = -step;
            if (step < std::numeric_limits<qint32>::min()
                || step > std::numeric_limits<qint32>::max()) {
                return failAndCleanup("X row-step pulse is outside the IMC60G int32 range.", errorMessage);
            }
            QString detail;
            if (!operation(motion_.stepX(static_cast<qint32>(step), job_.config.axisX, &detail),
                    "X row step failed to start.", detail, errorMessage)) {
                return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
            }
            if (cancelRequested_.load()) return failAndCleanup("Print job was cancelled.", errorMessage);
            if (!operation(motion_.waitXStopped(timeoutMs, cancelRequested_, &detail),
                    "X row step did not verify stopped.", detail, errorMessage)) {
                return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
            }
            if (cancelRequested_.load()) return failAndCleanup("Print job was cancelled.", errorMessage);
            pendingXStep_ = false;
        }

        const V2RowPlan& row = job_.plan.rows.at(nextRow_);
        activeFlowRow_ = row.row;
        QVector<PrintFrame> rowFrames;
        QVector<QSize> targetSizes;
        QString detail;
        if (!prepareRow(row, &rowFrames, &targetSizes, &detail))
            return failAndCleanup(detail, errorMessage);

        // V2 uploads the complete row cache on the print thread before any
        // exposure arm, row anchor, or Y motion.  The paced display worker
        // must only issue VBlank/Present calls for already cached textures.
        if (!operation(presenter_.prepareRow(rowFrames, targetSizes, &detail),
                "Complete V2 row cache preparation failed.", detail, errorMessage)) {
            return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
        }
        // V2 warms its first cached image and consumes one physical VBlank
        // before row one.  This keeps device/swap-chain initialization out of
        // the first motion phase.
        if (!presenterWarmupDone_) {
            if (!operation(presenter_.presentPreparedRowFrame(0, &detail),
                    "V2 display warm-up Present failed.", detail, errorMessage)
                || !operation(presenter_.waitForDisplayFrame(&detail),
                    "V2 display warm-up VBlank failed.", detail, errorMessage)) {
                return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
            }
            presenterWarmupDone_ = true;
        }

        const int framesPerImage = row.holdFramesAfterPresent + 1;
        const QVector<V2RefreshSubmission> submissions =
            buildV2RefreshSubmissions(row.logicalFrameOrder.size(),
                framesPerImage, row.reverse, &detail);
        if (submissions.isEmpty())
            return failAndCleanup(detail, errorMessage);

        const auto prepareMotionBeforeDisplayAnchor = [&](QString* readyError) {
            emit rowDirectionChanged(row.reverse);
            if (cancelRequested_.load()) {
                if (readyError) *readyError = "Print job was cancelled.";
                return false;
            }
            QString readyDetail;
            if (!operation(exposure_.arm(static_cast<qint32>(row.compareBegin),
                    static_cast<qint32>(row.compareEnd), &readyDetail),
                    "SV660N exposure-window arm failed.", readyDetail, readyError)) {
                return false;
            }
            flowExposureActive_ = true;
            if (!operation(motion_.prepareYScan(static_cast<qint32>(row.yTarget),
                    job_.config.axisY, &readyDetail),
                    "Y scan preparation failed.", readyDetail, readyError)) {
                return false;
            }
            QString counterDetail;
            const bool counterPrepared = presenter_.prepareRowVBlankCounter(&counterDetail);
            if (flowLogger_) {
                const PrintRowVBlankCounterSnapshot counter =
                    presenter_.rowVBlankCounterSnapshot();
                flowLogger_->log(QString(
                    "event=vblank_counter_prepare row=%1 dir=%2 ready=%3 "
                    "adapter=%4 output_index=%5")
                    .arg(row.row + 1)
                    .arg(directionName(row.reverse))
                    .arg(counterPrepared ? 1 : 0)
                    .arg(counter.adapterIndex)
                    .arg(counter.outputIndex));
            }
            if (flowLogger_) {
                flowLogger_->log(QString(
                    "event=exposure_on row=%1 dir=%2 phase=constant "
                    "compare_start=%3 compare_end=%4 hardware_window_active=1 "
                    "source=armed_compare_window")
                    .arg(row.row + 1)
                    .arg(directionName(row.reverse))
                    .arg(row.compareBegin)
                    .arg(row.compareEnd));
            }
            return true;
        };

        qint64 rowMotionStartUs = 0;
        const auto startMotionAfterDisplayPrime = [&](qint64 anchorQpcUs,
                                                       QString* readyError) {
            if (cancelRequested_.load()) {
                if (readyError) *readyError = "Print job was cancelled.";
                return false;
            }
            QString readyDetail;
            rowMotionStartUs = runClock_.nsecsElapsed() / 1000;
            if (!operation(motion_.startPreparedYScan(&readyDetail),
                    "Y scan failed to start.", readyDetail, readyError)) {
                return false;
            }
            const qint64 motionStartEndUs = runClock_.nsecsElapsed() / 1000;
            flowMotionActive_ = true;
            if (flowLogger_) {
                flowLogger_->log(QString(
                    "event=motion_start_vblank_sync row=%1 dir=%2 "
                    "anchor_qpc_us=%3 start_qpc_us=%4 "
                    "start_after_vblank_us=%5 start_call_cost_us=%6 "
                    "source=v2_row_vblank_counter_anchor")
                    .arg(row.row + 1)
                    .arg(directionName(row.reverse))
                    .arg(anchorQpcUs)
                    .arg(rowMotionStartUs)
                    .arg(rowMotionStartUs - anchorQpcUs)
                    .arg(motionStartEndUs - rowMotionStartUs));
                flowLogger_->log(QString(
                    "event=motion_y_start row=%1 dir=%2 pulse=%3 "
                    "y_pos_before=%4 y_target_after=%5 elapsed_run_us=%6")
                    .arg(row.row + 1)
                    .arg(directionName(row.reverse))
                    .arg(row.yTarget - row.yStart)
                    .arg(row.yStart)
                    .arg(row.yTarget)
                    .arg(rowMotionStartUs));
                const qint64 pulsePerRefreshFrame =
                    framesPerImage > 0
                    ? job_.plan.stepPulse / framesPerImage : 0;
                flowLogger_->log(QString(
                    "event=vsync_display_config row=%1 dir=%2 "
                    "yConstBeginPos=%3 frames_per_image=%4 "
                    "displayDelayPulse=%5 startDelayFrames=%6 "
                    "effectiveDisplayDelayPulse=%7 pulsePerRefreshFrame=%8 "
                    "hold_frames_after_present=%9 print_thread_wait_vsync=0 "
                    "display_mode=verified_output_vblank_sequence")
                    .arg(row.row + 1)
                    .arg(directionName(row.reverse))
                    .arg(row.constantBegin)
                    .arg(framesPerImage)
                    .arg(row.effectiveDisplayDelayPulse)
                    .arg(row.startDelayFrames)
                    .arg(row.startDelayFrames * pulsePerRefreshFrame)
                    .arg(pulsePerRefreshFrame)
                    .arg(row.holdFramesAfterPresent));
            }
            if (cancelRequested_.load()) {
                if (readyError) *readyError = "Print job was cancelled.";
                return false;
            }
            return true;
        };

        if (!runV2DisplaySequence(row, rowFrames, targetSizes, submissions,
                prepareMotionBeforeDisplayAnchor, startMotionAfterDisplayPrime, &detail))
            return failAndCleanup(detail, errorMessage);

        presenter_.stopRowVBlankCounter();
        const PrintRowVBlankCounterSnapshot rowCounter =
            presenter_.rowVBlankCounterSnapshot();
        if (flowLogger_) {
            flowLogger_->log(QString(
                "event=row_vblank_summary row=%1 dir=%2 frame_zero_qpc_us=%3 "
                "vblank_frames_counted=%4 last_vblank_qpc_us=%5 last_wait_hr=0x%6")
                .arg(row.row + 1)
                .arg(directionName(row.reverse))
                .arg(rowCounter.frameZeroQpcUs)
                .arg(rowCounter.frameSnapshot)
                .arg(rowCounter.lastVBlankQpcUs)
                .arg(static_cast<quint32>(rowCounter.lastWaitHresult), 8, 16,
                    QLatin1Char('0')));
        }
        detail.clear();
        if (!operation(exposure_.disarm(&detail),
                "SV660N exposure disarm failed.", detail, errorMessage)) {
            return failAndCleanup(errorMessage ? *errorMessage : detail,
                errorMessage);
        }
        flowExposureActive_ = false;
        if (flowLogger_) {
            flowLogger_->log(QString(
                "event=exposure_off row=%1 phase=constant_end "
                "cols_sent=%2 exposure_open_count=1 expose_end_pos=%3 "
                "hardware_window_active=0")
                .arg(row.row + 1)
                .arg(row.logicalFrameOrder.size())
                .arg(row.exposureBegin + job_.plan.exposurePulse));
        }
        if (!operation(motion_.waitYStopped(timeoutMs, cancelRequested_, &detail),
                "Y scan did not verify stopped.", detail, errorMessage)) {
            return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
        }
        flowMotionActive_ = false;
        if (flowLogger_) {
            flowLogger_->log(QString(
                "event=motion_y_stop row=%1 elapsed_row_us=%2 "
                "y_pos_after=%3 cols_sent=%4 images_shown=%5 "
                "hardware_window_active=%6")
                .arg(row.row + 1)
                .arg(runClock_.nsecsElapsed() / 1000 - rowMotionStartUs)
                .arg(row.yTarget)
                .arg(row.logicalFrameOrder.size())
                .arg(row.logicalFrameOrder.size())
                .arg(flowExposureActive_ ? 1 : 0));
        }
        if (cancelRequested_.load()) return failAndCleanup("Print job was cancelled.", errorMessage);

        ++nextRow_;
        const bool hasNextRow = nextRow_ < job_.plan.rows.size();
        pendingXStep_ = hasNextRow;
        emit progressChanged(nextRow_ * 100 / job_.plan.rows.size(),
            QString("Completed row %1/%2.").arg(nextRow_).arg(job_.plan.rows.size()));

        if (pauseRequested_.load() && hasNextRow) {
            if (imageQueue_) imageQueue_->stop();
            if (!operation(motion_.stopMappedAxes(&detail), "Pause failed to stop X/Y.", detail, errorMessage)
                || !operation(motion_.waitMappedAxesStopped(timeoutMs, &detail),
                    "Pause did not verify X/Y stopped.", detail, errorMessage)) {
                return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
            }
            setState(PrintJobState::Paused);
            return true;
        }
    }
    return finishAndCleanup(true, "Print job completed.", errorMessage);
}

bool PrintJobRunner::failAndCleanup(const QString& primaryError,
    QString* errorMessage)
{
    return finishAndCleanup(false,
        primaryError.isEmpty() ? "Print job failed." : primaryError, errorMessage);
}

bool PrintJobRunner::cleanup(QString* errorMessage)
{
    stopImageQueue();
    presenter_.stopRowVBlankCounter();
    QString errors;
    QString detail;
    const bool disarmed = exposure_.disarm(&detail);
    if (!disarmed) appendError(&errors, "Exposure disarm failed: " + detail);
    if (disarmed && flowExposureActive_) {
        if (flowLogger_) {
            flowLogger_->log(QString(
                "event=exposure_off row=%1 phase=safety_cleanup "
                "hardware_window_active=0")
                .arg(activeFlowRow_ + 1));
        }
        flowExposureActive_ = false;
    }
    detail.clear();
    const bool stopped = motion_.stopMappedAxes(&detail);
    if (!stopped) appendError(&errors, "Stopping X/Y failed: " + detail);
    detail.clear();
    const bool stopVerified = motion_.waitMappedAxesStopped(boundedTimeoutMs(job_), &detail);
    if (!stopVerified) appendError(&errors, "X/Y stopped verification failed: " + detail);
    if (stopped && stopVerified && flowMotionActive_) {
        if (flowLogger_) {
            flowLogger_->log(QString(
                "event=motion_y_stop row=%1 phase=safety_cleanup "
                "hardware_window_active=0")
                .arg(activeFlowRow_ + 1));
        }
        flowMotionActive_ = false;
    }

    bool zeroReturned = false;
    bool zeroVerified = false;
    if (disarmed && stopped && stopVerified) {
        detail.clear();
        zeroReturned = motion_.returnToLogicalZero(job_.config.axisX,
            job_.config.axisY, boundedTimeoutMs(job_), &detail);
        if (!zeroReturned) appendError(&errors, "Returning X/Y to logical zero failed: " + detail);
        if (zeroReturned) {
            detail.clear();
            zeroVerified = motion_.verifyLogicalZero(&detail);
            if (!zeroVerified) appendError(&errors, "X/Y zero/stopped verification failed: " + detail);
        }
    }
    detail.clear();
    if (!presenter_.shutdownChecked(&detail))
        appendError(&errors, "Presenter shutdown failed: " + detail);
    presenterPrepared_ = false;
    presenterWarmupDone_ = false;
    if (printOwnership_) {
        motion_.endPrint();
        printOwnership_ = false;
    }
    if (errorMessage) *errorMessage = errors;
    return disarmed && stopped && stopVerified && zeroReturned && zeroVerified
        && errors.isEmpty();
}

bool PrintJobRunner::finishAndCleanup(bool success, const QString& message,
    QString* errorMessage)
{
    setState(PrintJobState::Stopping);
    if (!success && flowLogger_) {
        flowLogger_->log(QString(
            "event=run_abort reason=%1 row=%2 detail=%3")
            .arg(cancelRequested_.load(std::memory_order_acquire)
                    ? QStringLiteral("cancelled")
                    : QStringLiteral("operation_failed"))
            .arg(qMax(1, activeFlowRow_ + 1))
            .arg(quotedFlowValue(message)));
    }
    QString cleanupError;
    const bool safe = cleanup(&cleanupError);
    QString finalMessage = message;
    if (!cleanupError.isEmpty()) appendError(&finalMessage, "Safety cleanup failed: " + cleanupError);
    const bool finalSuccess = success && safe;
    const bool safelyCancelled = !success && cancelRequested_.load() && safe;
    if (success && !safe && flowLogger_) {
        flowLogger_->log(QString(
            "event=run_abort reason=cleanup_failed row=%1 detail=%2")
            .arg(qMax(1, activeFlowRow_ + 1))
            .arg(quotedFlowValue(cleanupError)));
    }
    if (flowLogger_) {
        flowLogger_->log(QString(
            "event=run_end reason=%1 total_rows=%2 elapsed_run_us=%3")
            .arg(finalSuccess ? QStringLiteral("completed")
                              : QStringLiteral("failed"))
            .arg(job_.plan.rows.size())
            .arg(runClock_.isValid()
                    ? runClock_.nsecsElapsed() / 1000 : 0));
        flowLogger_->flush(2000);
    }
    setState((finalSuccess || safelyCancelled)
            ? PrintJobState::Ready : PrintJobState::Fault);
    if (!success && cancelRequested_.load(std::memory_order_acquire)) {
        emit progressChanged(0, safe
                ? QStringLiteral("Print job cancelled; cleanup verified.")
                : QStringLiteral("Print job cancelled; cleanup requires attention."));
    }
    if (errorMessage) *errorMessage = finalSuccess ? QString() : finalMessage;
    emit finished(finalSuccess, finalMessage);
    return finalSuccess;
}

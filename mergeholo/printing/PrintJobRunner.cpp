#include "PrintJobRunner.h"

#include <QtMath>
#include <QThread>

#include <cmath>
#include <limits>

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
    const qint64 width = qRound64(widthValue);
    const qint64 height = qRound64(heightValue);
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

} // namespace

PrintJobRunner::PrintJobRunner(IMotionController& motion,
    IExposureController& exposure, IPrintFramePresenter& presenter,
    IPrintHardwarePreflight& preflight, QObject* parent)
    : QObject(parent)
    , motion_(motion)
    , exposure_(exposure)
    , presenter_(presenter)
    , preflight_(preflight)
{
}

PrintJobRunner::~PrintJobRunner()
{
    const PrintJobState current = state();
    if (current == PrintJobState::Printing || current == PrintJobState::Paused
        || current == PrintJobState::Stopping) {
        cancelRequested_.store(true);
        QString ignored;
        cleanup(&ignored);
        state_.store(PrintJobState::Fault);
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
    frames_.clear();
    frames_.reserve(static_cast<int>(job_.images.imageCount()));
    for (size_t index = 0; index < job_.images.imageCount(); ++index) {
        PrintFrame frame;
        QString detail;
        if (!job_.images.copyFrame(index, &frame, &detail)) {
            setState(PrintJobState::Fault);
            if (errorMessage) *errorMessage = detail;
            emit finished(false, detail);
            return false;
        }
        frames_.append(std::move(frame));
    }
    QString detail;
    if (!motion_.beginPrint(&detail)) {
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
        const int anchorColumn = row.logicalFrameOrder.first();
        const int anchorIndex = row.row * job_.config.main.gridColumns + anchorColumn;
        const PrintFrame& anchorFrame = frames_.at(anchorIndex);
        QString detail;
        const QSize anchorTarget = targetSize(anchorFrame, job_.config, &detail);
        if (anchorTarget.isEmpty()) return failAndCleanup(detail, errorMessage);
        const bool anchorPresented = !presenterPrepared_
            ? presenter_.prepare(anchorFrame, anchorTarget, &detail)
            : presenter_.presentRowAnchor(anchorFrame, anchorTarget, &detail);
        presenterPrepared_ = presenterPrepared_ || anchorPresented;
        if (!operation(anchorPresented, "Row-anchor Present failed.", detail, errorMessage))
            return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
        if (cancelRequested_.load()) return failAndCleanup("Print job was cancelled.", errorMessage);

        PrintRowVBlankAnchor anchor;
        if (!operation(presenter_.acquireRowAnchor(&anchor, &detail),
                "Physical row VBlank anchor failed.", detail, errorMessage))
            return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
        if (cancelRequested_.load()) return failAndCleanup("Print job was cancelled.", errorMessage);

        emit rowDirectionChanged(row.reverse);
        if (!operation(exposure_.arm(static_cast<qint32>(row.compareBegin),
                static_cast<qint32>(row.compareEnd), &detail),
                "SV660N exposure-window arm failed.", detail, errorMessage)) {
            return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
        }
        if (cancelRequested_.load()) return failAndCleanup("Print job was cancelled.", errorMessage);
        if (!operation(motion_.startYScan(static_cast<qint32>(row.yTarget),
                job_.config.axisY, &detail),
                "Y scan failed to start.", detail, errorMessage)) {
            return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
        }
        if (cancelRequested_.load()) return failAndCleanup("Print job was cancelled.", errorMessage);

        int slot = 0;
        for (int delay = 0; delay < row.startDelayFrames; ++delay) {
            if (!operation(presenter_.waitForRowSlot(slot++, &anchor, &detail),
                    "Row start-delay VBlank failed.", detail, errorMessage))
                return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
            if (cancelRequested_.load()) return failAndCleanup("Print job was cancelled.", errorMessage);
        }

        for (int column = 0; column < row.logicalFrameOrder.size(); ++column) {
            const int logicalFrame = row.logicalFrameOrder.at(column);
            const int imageIndex = row.row * job_.config.main.gridColumns + logicalFrame;
            const PrintFrame& frame = frames_.at(imageIndex);
            if (column > 0) {
                const QSize size = targetSize(frame, job_.config, &detail);
                if (size.isEmpty()
                    || !operation(presenter_.present(frame, size, &detail),
                        "Print-frame Present failed.", detail, errorMessage))
                    return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
                if (cancelRequested_.load())
                    return failAndCleanup("Print job was cancelled.", errorMessage);
            }
            emit frameAdvanced(row.row, column, logicalFrame);
            if (!operation(presenter_.waitForRowSlot(slot++, &anchor, &detail),
                    "Print-frame physical VBlank failed.", detail, errorMessage))
                return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
            for (int hold = 0; hold < row.holdFramesAfterPresent; ++hold) {
                if (!operation(presenter_.waitForRowSlot(slot++, &anchor, &detail),
                        "Print-frame hold VBlank failed.", detail, errorMessage))
                    return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
            }
            if (cancelRequested_.load()) return failAndCleanup("Print job was cancelled.", errorMessage);
        }

        if (!operation(motion_.waitYStopped(timeoutMs, cancelRequested_, &detail),
                "Y scan did not verify stopped.", detail, errorMessage)
            || !operation(exposure_.disarm(&detail),
                "SV660N exposure disarm failed.", detail, errorMessage)) {
            return failAndCleanup(errorMessage ? *errorMessage : detail, errorMessage);
        }
        if (cancelRequested_.load()) return failAndCleanup("Print job was cancelled.", errorMessage);

        ++nextRow_;
        const bool hasNextRow = nextRow_ < job_.plan.rows.size();
        pendingXStep_ = hasNextRow;
        emit progressChanged(nextRow_ * 100 / job_.plan.rows.size(),
            QString("Completed row %1/%2.").arg(nextRow_).arg(job_.plan.rows.size()));

        if (pauseRequested_.load() && hasNextRow) {
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
    QString errors;
    QString detail;
    const bool disarmed = exposure_.disarm(&detail);
    if (!disarmed) appendError(&errors, "Exposure disarm failed: " + detail);
    detail.clear();
    const bool stopped = motion_.stopMappedAxes(&detail);
    if (!stopped) appendError(&errors, "Stopping X/Y failed: " + detail);
    detail.clear();
    const bool stopVerified = motion_.waitMappedAxesStopped(boundedTimeoutMs(job_), &detail);
    if (!stopVerified) appendError(&errors, "X/Y stopped verification failed: " + detail);

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
    QString cleanupError;
    const bool safe = cleanup(&cleanupError);
    QString finalMessage = message;
    if (!cleanupError.isEmpty()) appendError(&finalMessage, "Safety cleanup failed: " + cleanupError);
    const bool finalSuccess = success && safe;
    const bool safelyCancelled = !success && cancelRequested_.load() && safe;
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

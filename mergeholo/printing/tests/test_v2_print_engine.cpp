#include "../PrintHardwarePreflight.h"
#include "../PrintFlowLogger.h"
#include "../PrintImageSource.h"
#include "../PrintJobRunner.h"
#include "../PrintPositionSampler.h"
#include "../V2RowDisplaySequence.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <functional>
#include <future>
#include <limits>
#include <mutex>
#include <thread>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {

void expect(bool condition, const QString& message)
{
    if (!condition) {
        qCritical().noquote() << "FAIL:" << message;
        std::fprintf(stderr, "FAIL: %s\n", message.toUtf8().constData());
        std::fflush(stderr);
        std::exit(1);
    }
}

PrintFrame frameWithId(int id)
{
    PrintFrame frame;
    frame.width = 1;
    frame.height = 1;
    frame.stride = 3;
    frame.format = PrintPixelFormat::Bgr24;
    frame.pixels = QByteArray(3, static_cast<char>(id));
    return frame;
}

class RecordingMotion : public IMotionController {
public:
    PrintMotionReadiness readiness;
    QStringList* trace = nullptr;
    std::mutex* traceMutex = nullptr;
    int yMoves = 0;
    int xMoves = 0;
    int cleanupStops = 0;
    int returnZeroCalls = 0;
    bool stopOk = true;
    bool waitMappedOk = true;
    bool returnZeroOk = true;
    bool verifyZeroOk = true;
    bool beginOk = true;
    bool yMoveOk = true;
    bool yPrepareOk = true;
    bool yWaitOk = true;
    bool xMoveOk = true;
    bool xWaitOk = true;
    bool scanningReverse = false;
    std::function<void()> onWaitY;
    std::atomic_int telemetrySampleCount {0};

    void appendTrace(const QString& event)
    {
        if (!trace) return;
        if (traceMutex) {
            std::lock_guard<std::mutex> lock(*traceMutex);
            trace->append(event);
            return;
        }
        trace->append(event);
    }

    PrintMotionReadiness printReadiness(QString*) const override { return readiness; }
    bool beginPrint(QString*) override { return beginOk; }
    void endPrint() override {}
    bool startYScan(qint32 target, const PrintAxisConfig&, QString*) override
    {
        ++yMoves;
        appendTrace(target > 0 ? "move_y:forward" : "move_y:reverse");
        return yMoveOk;
    }
    bool prepareYScan(qint32 target, const PrintAxisConfig&, QString*) override
    {
        scanningReverse = target == 0;
        telemetrySampleCount.store(0);
        appendTrace(target > 0 ? "prepare_y:forward" : "prepare_y:reverse");
        return yPrepareOk;
    }
    bool startPreparedYScan(QString*) override
    {
        ++yMoves;
        appendTrace("start_prepared_y");
        return yMoveOk;
    }
    bool waitYStopped(int, const std::atomic_bool&, QString*) override
    {
        appendTrace("wait_y");
        if (onWaitY) onWaitY();
        return yWaitOk;
    }
    bool stepX(qint32, const PrintAxisConfig&, QString*) override
    {
        ++xMoves;
        appendTrace("move_x");
        return xMoveOk;
    }
    bool waitXStopped(int, const std::atomic_bool&, QString*) override
    {
        appendTrace("wait_x");
        return xWaitOk;
    }
    bool stopMappedAxes(QString*) override
    {
        ++cleanupStops;
        appendTrace("cleanup_stop");
        return stopOk;
    }
    bool waitMappedAxesStopped(int, QString*) override
    {
        appendTrace("cleanup_wait_stopped");
        return waitMappedOk;
    }
    bool returnToLogicalZero(const PrintAxisConfig&, const PrintAxisConfig&, int, QString*) override
    {
        ++returnZeroCalls;
        appendTrace("return_xy_zero");
        return returnZeroOk;
    }
    bool verifyLogicalZero(QString*) override { return verifyZeroOk; }
    PrintMappedAxisTelemetry sampleMappedAxisTelemetry() override
    {
        PrintMappedAxisTelemetry telemetry;
        telemetry.available = true;
        const int sample = telemetrySampleCount.fetch_add(1);
        telemetry.yPlannedPosition = sample < 64
            ? 16000
            : (scanningReverse ? 0 : std::numeric_limits<qint32>::max());
        return telemetry;
    }
};

class PositionGateMotion final : public RecordingMotion {
public:
    QVector<qint32> plannedYPositions;
    std::atomic_int nextPosition {0};
    std::thread::id firstTelemetrySampleThread;

    PrintMappedAxisTelemetry sampleMappedAxisTelemetry() override
    {
        if (firstTelemetrySampleThread == std::thread::id()) {
            firstTelemetrySampleThread = std::this_thread::get_id();
        }
        const int index = nextPosition.fetch_add(1);
        const qint32 yPosition = plannedYPositions.at(
            qMin(index, plannedYPositions.size() - 1));
        appendTrace("sample_y:" + QString::number(yPosition));
        PrintMappedAxisTelemetry telemetry;
        telemetry.available = true;
        telemetry.yPlannedPosition = yPosition;
        return telemetry;
    }
};

class RecordingExposure final : public IExposureController {
public:
    PrintExposureReadiness readiness;
    QStringList* trace = nullptr;
    std::mutex* traceMutex = nullptr;
    int armCalls = 0;
    int disarmCalls = 0;
    bool disarmOk = true;
    bool armOk = true;
    std::function<void()> onDisarm;

    void appendTrace(const QString& event)
    {
        if (!trace) return;
        if (traceMutex) {
            std::lock_guard<std::mutex> lock(*traceMutex);
            trace->append(event);
            return;
        }
        trace->append(event);
    }

    PrintExposureReadiness printReadiness(QString*) const override { return readiness; }
    bool arm(qint32, qint32, QString*) override
    {
        ++armCalls;
        appendTrace(lastReverse ? "arm:reverse" : "arm:forward");
        return armOk;
    }
    bool disarm(QString*) override
    {
        ++disarmCalls;
        if (armCalls > 0 && disarmCalls <= armCalls) appendTrace("disarm");
        if (onDisarm) onDisarm();
        return disarmOk;
    }
    bool isArmed() const override { return false; }
    bool lastReverse = false;
};

class RecordingPresenter final : public IPrintFramePresenter {
public:
    PrintPresenterReadiness readiness;
    QStringList* trace = nullptr;
    std::mutex* traceMutex = nullptr;
    int prepareCalls = 0;
    int presentCalls = 0;
    int shutdownCalls = 0;
    bool presentOk = true;
    bool shutdownOk = true;
    bool prepareOk = true;
    bool rowAnchorOk = true;
    bool acquireOk = true;
    bool waitDisplayOk = true;
    bool waitSlotOk = true;
    bool prepareRowOk = true;
    bool preparedPresentOk = true;
    bool recordRowVBlankCounter = false;
    int preparedPresentFailureCall = -1;
    int waitSlotFailureCall = -1;
    int prepareRowCalls = 0;
    int preparedPresentCalls = 0;
    int preparedPresentCallsThisRow = 0;
    int waitDisplayCalls = 0;
    int waitSlotCalls = 0;
    int frameStatisticsSampleCalls = 0;
    bool traceLegacyAnchor = true;
    std::atomic_bool displayWorkerActive {false};
    std::thread::id displayWorkerThread;
    std::thread::id prepareRowThread;
    std::thread::id primeThread;
    std::thread::id anchorThread;
    int displayWorkerPriority = 0;
    QVector<QSize> lastPreparedTargetSizes;
    std::function<void()> onPresent;
    std::function<void()> onPreparedPresent;
    std::function<void()> onShutdown;

    void appendTrace(const QString& event)
    {
        if (!trace) return;
        if (traceMutex) {
            std::lock_guard<std::mutex> lock(*traceMutex);
            trace->append(event);
            return;
        }
        trace->append(event);
    }

    PrintPresenterReadiness printReadiness(QString*) const override { return readiness; }
    bool prepare(const PrintFrame& frame, const QSize&, QString*) override
    {
        ++prepareCalls;
        if (traceLegacyAnchor) {
            appendTrace("present_anchor:" + QString::number(
                static_cast<unsigned char>(frame.pixels[0])));
        }
        return prepareOk;
    }
    bool presentRowAnchor(const PrintFrame& frame, const QSize&, QString*) override
    {
        if (traceLegacyAnchor) {
            appendTrace("present_anchor:" + QString::number(
                static_cast<unsigned char>(frame.pixels[0])));
        }
        return rowAnchorOk;
    }
    bool present(const PrintFrame&, const QSize&, QString*) override
    {
        ++presentCalls;
        if (onPresent) onPresent();
        return presentOk;
    }
    bool prepareRow(const QVector<PrintFrame>& frames, const QVector<QSize>& sizes,
        QString*) override
    {
        prepareRowThread = std::this_thread::get_id();
#ifdef Q_OS_WIN
        displayWorkerPriority = GetThreadPriority(GetCurrentThread());
#endif
        ++prepareRowCalls;
        lastPreparedTargetSizes = sizes;
        preparedPresentCallsThisRow = 0;
        if (!frames.isEmpty()) {
            appendTrace("prepare_row:" + QString::number(
                static_cast<unsigned char>(frames.first().pixels[0])));
        }
        return prepareRowOk && !frames.isEmpty() && frames.size() == sizes.size();
    }
    bool presentPreparedRowFrame(int logicalFrame, QString*) override
    {
        displayWorkerActive.store(true, std::memory_order_release);
        if (preparedPresentCallsThisRow == 0)
            primeThread = std::this_thread::get_id();
        if (preparedPresentCallsThisRow > 0)
            displayWorkerThread = std::this_thread::get_id();
        const int callThisRow = preparedPresentCallsThisRow++;
        const int call = preparedPresentCalls++;
        appendTrace(QString(callThisRow == 0 ? "prime:%1" : "cached_present:%1")
            .arg(logicalFrame));
        if (callThisRow > 0 && onPreparedPresent) onPreparedPresent();
        const bool ok = preparedPresentOk
            && (preparedPresentFailureCall < 0
                || call != preparedPresentFailureCall);
        displayWorkerActive.store(false, std::memory_order_release);
        return ok;
    }
    void clearPreparedRow() override {}
    bool waitForDisplayFrame(QString*) override
    {
        ++waitDisplayCalls;
        appendTrace("vblank");
        return waitDisplayOk;
    }
    bool acquireRowAnchor(PrintRowVBlankAnchor* anchor, QString*) override
    {
        anchorThread = std::this_thread::get_id();
        anchor->available = true;
        appendTrace("vblank_anchor");
        return acquireOk;
    }
    bool waitForRowSlot(int slot, PrintRowVBlankAnchor* anchor, QString*) override
    {
        displayWorkerActive.store(true, std::memory_order_release);
        displayWorkerThread = std::this_thread::get_id();
#ifdef Q_OS_WIN
        displayWorkerPriority = GetThreadPriority(GetCurrentThread());
#endif
        const int call = waitSlotCalls++;
        if (prepareRowCalls > 0 && slot == 0 && anchor && anchor->available) {
            anchor->available = false;
        } else {
            appendTrace("vblank");
        }
        const bool ok = waitSlotOk
            && (waitSlotFailureCall < 0 || call != waitSlotFailureCall);
        displayWorkerActive.store(false, std::memory_order_release);
        return ok;
    }
    void sampleFrameStatistics() override
    {
        ++frameStatisticsSampleCalls;
    }
    bool prepareRowVBlankCounter(QString*) override
    {
        if (recordRowVBlankCounter) appendTrace("vblank_counter_prepare");
        return true;
    }
    void beginRowVBlankCounter(qint64) override
    {
        if (recordRowVBlankCounter) appendTrace("vblank_counter_begin");
    }
    void stopRowVBlankCounter() override
    {
        if (recordRowVBlankCounter) appendTrace("vblank_counter_stop");
    }
    void shutdown() override
    {
        ++shutdownCalls;
        appendTrace("presenter_shutdown");
        if (onShutdown) onShutdown();
    }
    bool shutdownChecked(QString* errorMessage) override
    {
        shutdown();
        if (!shutdownOk && errorMessage) *errorMessage = "injected shutdown failure";
        return shutdownOk;
    }
};

class RecordingPreflight final : public IPrintHardwarePreflight {
public:
    QStringList* trace = nullptr;
    PrintPreflightResult result;
    PrintPreflightResult check(const PrintJobSnapshot&, bool dynamicOnly) override
    {
        if (trace) trace->append(dynamicOnly ? "preflight_dynamic" : "preflight");
        return result;
    }
};

class RecordingFlowLogger final : public IPrintFlowLogger {
public:
    void log(const QString& record) noexcept override
    {
        std::lock_guard<std::mutex> lock(mutex);
        records.append(record);
        writerThreads.append(std::this_thread::get_id());
    }

    bool flush(int) override { return true; }

    QStringList snapshot() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return records;
    }

    QVector<std::thread::id> threadSnapshot() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return writerThreads;
    }

private:
    mutable std::mutex mutex;
    QStringList records;
    QVector<std::thread::id> writerThreads;
};

struct QueueProbe {
    QStringList* trace = nullptr;
    QVector<PrintFrame> frames;
    int nextIndex = 0;
    int startCalls = 0;
    int takeRowCalls = 0;
    int stopCalls = 0;
};

class RecordingImageQueue final : public IPrintImageQueue {
public:
    explicit RecordingImageQueue(std::shared_ptr<QueueProbe> probe)
        : probe_(std::move(probe))
    {
    }

    bool start(QString*) override
    {
        ++probe_->startCalls;
        if (probe_->trace) probe_->trace->append("queue_start");
        return true;
    }

    bool takeRow(int columnCount, QVector<PrintFrame>* row, QString* errorMessage) override
    {
        ++probe_->takeRowCalls;
        if (probe_->trace) probe_->trace->append("queue_take_row");
        if (!row || columnCount <= 0
            || probe_->nextIndex + columnCount > probe_->frames.size()) {
            if (errorMessage) *errorMessage = "injected queue row unavailable";
            return false;
        }
        row->clear();
        row->reserve(columnCount);
        for (int column = 0; column < columnCount; ++column)
            row->append(probe_->frames.at(probe_->nextIndex++));
        return true;
    }

    void stop() override
    {
        ++probe_->stopCalls;
        if (probe_->trace) probe_->trace->append("queue_stop_join");
    }

private:
    std::shared_ptr<QueueProbe> probe_;
};

PrintImageQueueFactory recordingQueueFactory(
    const std::shared_ptr<QueueProbe>& probe)
{
    return [probe](const PrintImageSet&) {
        return std::make_unique<RecordingImageQueue>(probe);
    };
}

PrintMotionReadiness readyMotion()
{
    PrintMotionReadiness value;
    value.sdkRuntimeReady = true;
    value.cardReady = true;
    value.ethercatReady = true;
    value.axisMappingLocked = true;
    value.servosReady = true;
    value.emergencyClear = true;
    value.axesHomed = true;
    value.axesStopped = true;
    return value;
}

PrintExposureReadiness readyExposure()
{
    PrintExposureReadiness value;
    value.profileMatches = true;
    value.safeBaseline = true;
    return value;
}

PrintPresenterReadiness readyPresenter()
{
    PrintPresenterReadiness value;
    value.secondScreenAttached = true;
    value.presenterAvailable = true;
    value.vblankReady = true;
    value.generationCurrent = true;
    return value;
}

PrintJobSnapshot makeJob(int rows, int columns)
{
    PrintJobSnapshot job;
    job.config = defaultPrint9030Config();
    job.config.main.gridRows = rows;
    job.config.main.gridColumns = columns;
    job.profile = PrintHardwareProfile();
    QString timingError;
    job.plan = buildV2PrintPlan(job.config, job.profile, 60.0, &timingError);
    expect(timingError.isEmpty(), timingError);
    for (V2RowPlan& row : job.plan.rows) row.startDelayFrames = 0;
    QVector<PrintFrame> frames;
    for (int index = 0; index < rows * columns; ++index) frames.append(frameWithId(index % columns));
    QString imageError;
    job.images = PrintImageSet::fromFrames(frames, &imageError);
    expect(imageError.isEmpty(), imageError);
    return job;
}

void testPreflightMatrixAndZeroUnsafeCalls()
{
    struct Case { PreflightFault fault; const char* text; };
    const Case cases[] = {
        {PreflightFault::SdkRuntime, "SDK"},
        {PreflightFault::Card, "Card0"},
        {PreflightFault::Ethercat, "EtherCAT"},
        {PreflightFault::AxisMapping, "X=1, Y=0"},
        {PreflightFault::Servo, "Servo"},
        {PreflightFault::Emergency, "emergency"},
        {PreflightFault::Homing, "home"},
        {PreflightFault::ExposureProfile, "SV660N"},
        {PreflightFault::ExposureBaseline, "baseline"},
        {PreflightFault::TimingPlan, "V2"},
        {PreflightFault::SecondScreen, "second screen"},
        {PreflightFault::Presenter, "presenter"},
        {PreflightFault::VBlank, "VBlank"}
    };

    for (const Case& item : cases) {
        RecordingMotion motion;
        RecordingExposure exposure;
        RecordingPresenter presenter;
        motion.readiness = readyMotion();
        exposure.readiness = readyExposure();
        presenter.readiness = readyPresenter();
        PrintJobSnapshot job = makeJob(1, 1);
        switch (item.fault) {
        case PreflightFault::SdkRuntime: motion.readiness.sdkRuntimeReady = false; break;
        case PreflightFault::Card: motion.readiness.cardReady = false; break;
        case PreflightFault::Ethercat: motion.readiness.ethercatReady = false; break;
        case PreflightFault::AxisMapping: motion.readiness.axisMappingLocked = false; break;
        case PreflightFault::Servo: motion.readiness.servosReady = false; break;
        case PreflightFault::Emergency: motion.readiness.emergencyClear = false; break;
        case PreflightFault::Homing: motion.readiness.axesHomed = false; break;
        case PreflightFault::ExposureProfile: exposure.readiness.profileMatches = false; break;
        case PreflightFault::ExposureBaseline: exposure.readiness.safeBaseline = false; break;
        case PreflightFault::TimingPlan: job.plan.rows.clear(); break;
        case PreflightFault::SecondScreen: presenter.readiness.secondScreenAttached = false; break;
        case PreflightFault::Presenter: presenter.readiness.presenterAvailable = false; break;
        case PreflightFault::VBlank: presenter.readiness.vblankReady = false; break;
        default: break;
        }
        PrintHardwarePreflight preflight(motion, exposure, presenter);
        PrintJobRunner runner(motion, exposure, presenter, preflight);
        QString error;
        expect(!runner.start(job, &error), "injected preflight failure must reject start");
        expect(error.contains(QString::fromUtf8(item.text), Qt::CaseInsensitive),
            QString("fault text '%1' missing from '%2'").arg(QString::fromUtf8(item.text), error));
        expect(motion.yMoves == 0 && motion.xMoves == 0, "preflight failure moved an axis");
        expect(exposure.armCalls == 0, "preflight failure armed exposure");
        expect(runner.state() == PrintJobState::Fault, "preflight failure must enter Fault");
    }
}

void testExactOneRowOrder()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    motion.readiness = readyMotion();
    exposure.readiness = readyExposure();
    presenter.readiness = readyPresenter();
    preflight.result.ok = true;
    QStringList trace;
    motion.trace = &trace;
    exposure.trace = &trace;
    presenter.trace = &trace;
    preflight.trace = &trace;
    PrintJobRunner runner(motion, exposure, presenter, preflight);
    QObject::connect(&runner, &PrintJobRunner::frameAdvanced,
        [&trace](int, int, int logicalFrame) { trace.append("frame:" + QString::number(logicalFrame)); });
    QObject::connect(&runner, &PrintJobRunner::rowDirectionChanged,
        [&exposure](bool reverse) { exposure.lastReverse = reverse; });
    QObject::connect(&runner, &PrintJobRunner::finished,
        [&trace](bool success, const QString&) { trace.append(success ? "finished_success" : "finished_failure"); });

    QString error;
    expect(runner.start(makeJob(1, 3), &error), error);
    const QStringList expected = {
        "preflight", "present_anchor:0", "prepare_row:0", "prime:0", "vblank", "arm:forward", "prepare_y:forward", "cached_present:0",
        "vblank_anchor", "start_prepared_y",
        "cached_present:1", "frame:0", "vblank", "cached_present:2",
        "frame:1", "vblank", "cached_present:2", "frame:2",
        "disarm", "wait_y", "cleanup_stop",
        "cleanup_wait_stopped",
        "return_xy_zero", "presenter_shutdown", "finished_success"
    };
    expect(trace == expected,
        QString("1-row event order mismatch actual=%1 expected=%2")
            .arg(trace.join(","), expected.join(",")));
    expect(motion.xMoves == 0, "one row must not perform an X row step");
}

void testExactOneRowAndTwoByThreeOrder()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    motion.readiness = readyMotion();
    exposure.readiness = readyExposure();
    presenter.readiness = readyPresenter();
    preflight.result.ok = true;
    QStringList trace;
    motion.trace = &trace;
    exposure.trace = &trace;
    presenter.trace = &trace;
    preflight.trace = &trace;
    PrintJobRunner runner(motion, exposure, presenter, preflight);
    QObject::connect(&runner, &PrintJobRunner::frameAdvanced,
        [&trace](int, int, int logicalFrame) { trace.append("frame:" + QString::number(logicalFrame)); });
    QObject::connect(&runner, &PrintJobRunner::rowDirectionChanged,
        [&exposure](bool reverse) { exposure.lastReverse = reverse; });
    QObject::connect(&runner, &PrintJobRunner::finished,
        [&trace](bool success, const QString&) { trace.append(success ? "finished_success" : "finished_failure"); });

    QString error;
    expect(runner.start(makeJob(2, 3), &error), error);
    const QStringList expected = {
        "preflight",
        "present_anchor:0", "prepare_row:0", "prime:0", "vblank", "arm:forward", "prepare_y:forward", "cached_present:0", "vblank_anchor",
        "start_prepared_y", "cached_present:1", "frame:0",
        "vblank", "cached_present:2", "frame:1", "vblank",
        "cached_present:2", "frame:2",
        "disarm", "wait_y", "move_x", "wait_x",
        "prepare_row:0", "arm:reverse", "prepare_y:reverse", "prime:2", "vblank_anchor",
        "start_prepared_y", "cached_present:1", "frame:2", "vblank",
        "cached_present:0", "frame:1", "vblank",
        "cached_present:0", "frame:0",
        "disarm", "wait_y", "cleanup_stop", "cleanup_wait_stopped",
        "return_xy_zero",
        "presenter_shutdown", "finished_success"
    };
    expect(trace == expected,
        QString("2x3 event order mismatch\nactual: %1\nexpected: %2")
            .arg(trace.join(","), expected.join(",")));
    expect(runner.state() == PrintJobState::Ready, "successful job must return Ready");
}

void testV2SourceCountAndFrameValidity()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    motion.readiness = readyMotion();
    exposure.readiness = readyExposure();
    presenter.readiness = readyPresenter();
    PrintHardwarePreflight preflight(motion, exposure, presenter);

    PrintJobSnapshot job = makeJob(2, 3);
    QVector<PrintFrame> shortFrames;
    for (int i = 0; i < 5; ++i) shortFrames.append(frameWithId(i));
    QString error;
    job.images = PrintImageSet::fromFrames(shortFrames, &error);
    expect(preflight.check(job, false).ok,
        "V2 preflight must not reject fewer images than the grid plan");

    QVector<PrintFrame> extraFrames;
    for (int i = 0; i < 7; ++i) extraFrames.append(frameWithId(i));
    job.images = PrintImageSet::fromFrames(extraFrames, &error);
    expect(preflight.check(job, false).ok,
        "V2 preflight must not reject extra images beyond the grid plan");

    QVector<PrintFrame> badFrames;
    for (int i = 0; i < 6; ++i) badFrames.append(frameWithId(i));
    badFrames[3].pixels.resize(2);
    job.images = PrintImageSet::fromFrames(badFrames, &error);
    expect(!job.images.isValid(), "truncated immutable frame set must be rejected at creation");
    expect(error.contains("frame", Qt::CaseInsensitive) || error.contains("image", Qt::CaseInsensitive),
        "invalid frame error must be actionable");
}

void testPreflightRejectsMalformedRowAddressing()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    motion.readiness = readyMotion();
    exposure.readiness = readyExposure();
    presenter.readiness = readyPresenter();
    PrintHardwarePreflight preflight(motion, exposure, presenter);

    const auto rejected = [&](const PrintJobSnapshot& candidate, const QString& label) {
        const PrintPreflightResult result = preflight.check(candidate, false);
        expect(!result.ok && result.fault == PreflightFault::TimingPlan,
            label + " must fail timing preflight");
    };

    PrintJobSnapshot job = makeJob(2, 3);
    job.plan.rows[1].row = 0;
    rejected(job, "duplicate row index");

    job = makeJob(2, 3);
    job.plan.rows[0].logicalFrameOrder = {0, 1, 3};
    rejected(job, "out-of-range logical frame");

    job = makeJob(2, 3);
    job.plan.rows[0].logicalFrameOrder = {0, 1, 1};
    rejected(job, "duplicate logical frame");
}

void testInvalidTargetScalesFailBeforeRounding()
{
    const double badScales[] = {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        static_cast<double>(std::numeric_limits<int>::max()) * 2.0
    };
    for (double scale : badScales) {
        RecordingMotion motion;
        RecordingExposure exposure;
        RecordingPresenter presenter;
        RecordingPreflight preflight;
        preflight.result.ok = true;
        PrintJobSnapshot job = makeJob(1, 1);
        job.config.main.widthScale = scale;
        PrintJobRunner runner(motion, exposure, presenter, preflight);
        QString error;
        expect(!runner.start(job, &error), "invalid target scale must fail closed");
        expect(error.contains("scale", Qt::CaseInsensitive),
            "invalid target scale must return actionable error");
        expect(presenter.prepareCalls == 0,
            "invalid target scale must not reach the presenter");
    }
}

void testPauseResumeRechecksAndDoesNotDuplicateFrames()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    motion.readiness = readyMotion();
    exposure.readiness = readyExposure();
    presenter.readiness = readyPresenter();
    preflight.result.ok = true;
    QStringList trace;
    motion.trace = &trace;
    exposure.trace = &trace;
    presenter.trace = &trace;
    preflight.trace = &trace;
    auto queueProbe = std::make_shared<QueueProbe>();
    queueProbe->trace = &trace;
    queueProbe->frames = {
        frameWithId(0), frameWithId(1), frameWithId(2),
        frameWithId(0), frameWithId(1), frameWithId(2)
    };
    PrintJobRunner runner(motion, exposure, presenter, preflight,
        recordingQueueFactory(queueProbe));
    QObject::connect(&runner, &PrintJobRunner::frameAdvanced,
        [&trace](int row, int, int logicalFrame) {
            trace.append(QString("row%1_frame:%2").arg(row).arg(logicalFrame));
        });
    QObject::connect(&runner, &PrintJobRunner::rowDirectionChanged,
        [&exposure](bool reverse) { exposure.lastReverse = reverse; });
    bool pauseIssued = false;
    motion.onWaitY = [&] {
        if (!pauseIssued) {
            pauseIssued = true;
            runner.requestPause();
        }
    };

    QString error;
    expect(runner.start(makeJob(2, 3), &error), error);
    expect(runner.state() == PrintJobState::Paused, "pause must take effect at row boundary");
    expect(trace.count("preflight") == 1, "initial preflight must run once");
    expect(trace.count("row0_frame:0") == 1 && trace.count("row0_frame:1") == 1
            && trace.count("row0_frame:2") == 1,
        "pause boundary must neither duplicate nor skip row zero");
    expect(queueProbe->startCalls == 1 && queueProbe->stopCalls == 1,
        "pause must stop/join the queue after the completed row");
    const int pauseQueueJoin = trace.indexOf("queue_stop_join");
    expect(pauseQueueJoin >= 0
            && pauseQueueJoin < trace.indexOf("cleanup_stop")
            && pauseQueueJoin < trace.indexOf("cleanup_wait_stopped"),
        "pause must stop/join the image queue before mapped-axis stop and wait commands");

    expect(runner.resume(&error), error);
    expect(trace.count("preflight_dynamic") == 1, "resume must rerun dynamic preflight");
    expect(trace.count("row1_frame:2") == 1 && trace.count("row1_frame:1") == 1
            && trace.count("row1_frame:0") == 1,
        "resume must neither duplicate nor skip reverse row");
    expect(queueProbe->startCalls == 2 && queueProbe->stopCalls == 2,
        "resume must restart the same queue position and completion must join it");
    expect(runner.state() == PrintJobState::Ready, "resumed completion must return Ready");
}

void testCancelDuringPresentDoesNotAdvanceAnotherVBlank()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    motion.readiness = readyMotion();
    exposure.readiness = readyExposure();
    presenter.readiness = readyPresenter();
    preflight.result.ok = true;
    QStringList trace;
    motion.trace = &trace;
    exposure.trace = &trace;
    presenter.trace = &trace;
    preflight.trace = &trace;
    PrintJobRunner runner(motion, exposure, presenter, preflight);
    QVector<int> progress;
    QObject::connect(&runner, &PrintJobRunner::frameAdvanced,
        [&trace](int, int, int logicalFrame) { trace.append("frame:" + QString::number(logicalFrame)); });
    QObject::connect(&runner, &PrintJobRunner::progressChanged,
        [&progress](int value, const QString&) { progress.append(value); });
    int preparedPresentCallbacks = 0;
    presenter.onPreparedPresent = [&] {
        // The first cached Present is V2's pre-row warm-up.  Inject the
        // cancellation in the actual paced sequence instead.
        if (preparedPresentCallbacks++ == 0) return;
        std::thread caller([&] { runner.cancel(); });
        caller.join();
    };

    QString error;
    expect(!runner.start(makeJob(1, 3), &error), "cancelled job must not report success");
    const int firstRegularPresent = trace.indexOf("frame:1");
    expect(firstRegularPresent < 0,
        "cancel accepted inside Present must not publish the cancelled frame");
    expect(trace.count("vblank") == 1,
        "cancel accepted inside Present must not wait beyond V2's one pre-row warm-up VBlank");
    expect(motion.cleanupStops == 1 && motion.returnZeroCalls == 1,
        "cancel must execute common safe cleanup");
    expect(runner.state() == PrintJobState::Ready,
        "safely cancelled connected hardware may return Ready");
    expect(!progress.isEmpty() && progress.last() == 0,
        "completed cancellation cleanup must reset progress to zero");
}

void testCancelledCleanupFailureStillResetsProgress()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    motion.readiness = readyMotion();
    exposure.readiness = readyExposure();
    presenter.readiness = readyPresenter();
    preflight.result.ok = true;
    motion.returnZeroOk = false;
    PrintJobRunner runner(motion, exposure, presenter, preflight);
    QVector<int> progress;
    QObject::connect(&runner, &PrintJobRunner::progressChanged,
        [&progress](int value, const QString&) { progress.append(value); });
    presenter.onPreparedPresent = [&] { runner.cancel(); };

    QString error;
    expect(!runner.start(makeJob(1, 2), &error),
        "cancelled job with failed zero return must not report success");
    expect(runner.state() == PrintJobState::Fault,
        "failed cancellation cleanup must remain Fault");
    expect(!progress.isEmpty() && progress.last() == 0,
        "failed cancellation cleanup must still reset progress to zero");
}

void testPausedCancelIsDrainedOnlyOnOwnerThread()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    preflight.result.ok = true;
    PrintJobRunner runner(motion, exposure, presenter, preflight);
    motion.onWaitY = [&] { runner.requestPause(); };

    QString error;
    expect(runner.start(makeJob(2, 1), &error), error);
    expect(runner.state() == PrintJobState::Paused, "job must pause at the row boundary");
    const int stopsBeforeCancel = motion.cleanupStops;
    std::thread caller([&] { runner.cancel(); });
    caller.join();
    expect(motion.cleanupStops == stopsBeforeCancel,
        "cross-thread cancel must not call cleanup or any SDK adapter");
    expect(!runner.processPendingControl(&error),
        "owner-thread drain of a paused cancel must report cancelled");
    expect(motion.cleanupStops == stopsBeforeCancel + 1,
        "owner-thread drain must execute cleanup exactly once");
    expect(runner.state() == PrintJobState::Ready,
        "safe paused cancellation may return Ready");
}

void testCleanupAggregatesFailuresAndStillShutsPresenterDown()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    motion.readiness = readyMotion();
    exposure.readiness = readyExposure();
    presenter.readiness = readyPresenter();
    preflight.result.ok = true;
    presenter.presentOk = false;
    presenter.shutdownOk = false;
    exposure.disarmOk = false;
    motion.stopOk = false;
    motion.waitMappedOk = false;
    PrintJobRunner runner(motion, exposure, presenter, preflight);

    QString error;
    expect(!runner.start(makeJob(1, 2), &error), "injected Present failure must fail the job");
    expect(error.contains("Present", Qt::CaseInsensitive), "primary Present failure was lost");
    expect(error.contains("disarm", Qt::CaseInsensitive), "disarm cleanup failure was not aggregated");
    expect(error.contains("Stopping", Qt::CaseInsensitive), "stop cleanup failure was not aggregated");
    expect(error.contains("stopped verification", Qt::CaseInsensitive),
        "wait-stopped cleanup failure was not aggregated");
    expect(error.contains("shutdown", Qt::CaseInsensitive),
        "presenter shutdown failure was not aggregated");
    expect(presenter.shutdownCalls == 1, "presenter shutdown must still be attempted once");
    expect(motion.returnZeroCalls == 0,
        "unsafe disarm/stop failure must prevent return-to-zero motion");
    expect(runner.state() == PrintJobState::Fault,
        "unverified cleanup must preserve Fault");
}

void testEveryEngineOperationFailureUsesCommonCleanup()
{
    using Configure = std::function<void(RecordingMotion&, RecordingExposure&,
        RecordingPresenter&, PrintJobSnapshot&)>;
    struct Case { const char* name; Configure configure; };
    const Case cases[] = {
        {"prepare", [](auto&, auto&, auto& p, auto&) { p.prepareOk = false; }},
        {"anchor", [](auto&, auto&, auto& p, auto&) { p.acquireOk = false; }},
        {"arm", [](auto&, auto& e, auto&, auto&) { e.armOk = false; }},
        {"y move", [](auto& m, auto&, auto&, auto&) { m.yMoveOk = false; }},
        {"start-delay VBlank", [](auto&, auto&, auto& p, auto& job) {
            p.waitSlotOk = false;
            job.plan.rows[0].startDelayFrames = 1;
        }},
        {"frame VBlank", [](auto&, auto&, auto& p, auto&) { p.waitSlotOk = false; }},
        {"frame Present", [](auto&, auto&, auto& p, auto&) {
            p.preparedPresentFailureCall = 1;
        }},
        {"y wait", [](auto& m, auto&, auto&, auto&) { m.yWaitOk = false; }},
        {"x move", [](auto& m, auto&, auto&, auto&) { m.xMoveOk = false; }},
        {"x wait", [](auto& m, auto&, auto&, auto&) { m.xWaitOk = false; }},
        {"next-row prime", [](auto&, auto&, auto& p, auto&) {
            p.preparedPresentFailureCall = 3;
        }}
    };

    for (const Case& item : cases) {
        RecordingMotion motion;
        RecordingExposure exposure;
        RecordingPresenter presenter;
        RecordingPreflight preflight;
        motion.readiness = readyMotion();
        exposure.readiness = readyExposure();
        presenter.readiness = readyPresenter();
        preflight.result.ok = true;
        PrintJobSnapshot job = makeJob(2, 2);
        item.configure(motion, exposure, presenter, job);
        PrintJobRunner runner(motion, exposure, presenter, preflight);
        QString error;
        expect(!runner.start(job, &error),
            QString("injected %1 failure must fail the job").arg(item.name));
        expect(runner.state() == PrintJobState::Fault,
            QString("injected %1 failure must preserve Fault").arg(item.name));
        expect(exposure.disarmCalls >= 1,
            QString("injected %1 failure skipped exposure disarm").arg(item.name));
        expect(motion.cleanupStops == 1,
            QString("injected %1 failure skipped mapped-axis stop").arg(item.name));
        expect(presenter.shutdownCalls == 1,
            QString("injected %1 failure skipped presenter shutdown").arg(item.name));
    }
}

void testEveryTerminalCleanupFailureIsUnsafe()
{
    using Configure = std::function<void(RecordingMotion&, RecordingPresenter&)>;
    struct Case { const char* name; const char* error; Configure configure; };
    const Case cases[] = {
        {"stop", "Stopping", [](auto& m, auto&) { m.stopOk = false; }},
        {"wait stopped", "stopped verification", [](auto& m, auto&) { m.waitMappedOk = false; }},
        {"return zero", "logical zero", [](auto& m, auto&) { m.returnZeroOk = false; }},
        {"verify zero", "zero/stopped", [](auto& m, auto&) { m.verifyZeroOk = false; }},
        {"shutdown", "shutdown", [](auto&, auto& p) { p.shutdownOk = false; }}
    };
    for (const Case& item : cases) {
        RecordingMotion motion;
        RecordingExposure exposure;
        RecordingPresenter presenter;
        RecordingPreflight preflight;
        motion.readiness = readyMotion();
        exposure.readiness = readyExposure();
        presenter.readiness = readyPresenter();
        preflight.result.ok = true;
        item.configure(motion, presenter);
        PrintJobRunner runner(motion, exposure, presenter, preflight);
        QString error;
        expect(!runner.start(makeJob(1, 1), &error),
            QString("cleanup %1 failure must reject success").arg(item.name));
        expect(error.contains(item.error, Qt::CaseInsensitive),
            QString("cleanup %1 failure missing from '%2'").arg(item.name, error));
        expect(runner.state() == PrintJobState::Fault,
            QString("cleanup %1 failure must preserve Fault").arg(item.name));
    }
}

void testIllegalRepeatedCommandsAndDestructionAreBounded()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    motion.readiness = readyMotion();
    exposure.readiness = readyExposure();
    presenter.readiness = readyPresenter();
    preflight.result.ok = true;
    {
        PrintJobRunner runner(motion, exposure, presenter, preflight);
        QString error;
        expect(!runner.resume(&error), "resume from Ready must be rejected");
        runner.requestPause();
        runner.cancel();
        runner.cancel();
        expect(runner.state() == PrintJobState::Ready,
            "repeated idle pause/cancel must not corrupt Ready");
    }
    expect(motion.cleanupStops == 0 && presenter.shutdownCalls == 0,
        "destroying an idle runner must not claim or perform job cleanup");
}

void testPrintPositionSamplingAndConversion()
{
    PrintPositionSampler sampler(100);
    expect(sampler.isDue(0), "first printing position sample must be due");
    expect(!sampler.isDue(99), "printing position sample must be throttled below 100 ms");
    expect(sampler.isDue(100), "printing position sample must be due at 100 ms");

    PrintAxisConfig xAxis;
    xAxis.subdivision = 40;
    xAxis.resolution = 50;
    PrintAxisConfig yAxis;
    yAxis.subdivision = 20;
    yAxis.resolution = 25;
    QPointF millimeters;
    expect(PrintPositionSampler::toMillimeters(12000, -34000, xAxis, yAxis,
            &millimeters),
        "valid planned pulses must convert to millimeters");
    expect(millimeters == QPointF(6.0, -68.0),
        "planned-position conversion must preserve axis signs and scales");

    yAxis.resolution = 0;
    expect(!PrintPositionSampler::toMillimeters(1, 1, xAxis, yAxis, &millimeters),
        "invalid pulse scale must reject position conversion");
}

void testPreflightDoesNotAddV2ImageCountGate()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    motion.readiness = readyMotion();
    exposure.readiness = readyExposure();
    presenter.readiness = readyPresenter();
    PrintJobSnapshot job = makeJob(1, 2);
    QString sourceError;
    job.images = PrintImageSet::fromFrames({frameWithId(1)}, &sourceError);
    expect(sourceError.isEmpty(), sourceError);
    PrintHardwarePreflight preflight(motion, exposure, presenter);
    const PrintPreflightResult result = preflight.check(job, false);
    expect(result.ok,
        "V2 preflight must not add an image-count-equals-grid gate");
}

void testV2RefreshSubmissionTrace()
{
    QString error;
    const auto forward = buildV2RefreshSubmissions(3, 2, false, &error);
    expect(error.isEmpty() && forward.size() == 6,
        "V2 forward sequence must have W*frames slots");
    expect(forward[0].visibleLogicalFrame == 0 && forward[0].submittedLogicalFrame == 0
            && forward[0].usesRowAnchor,
        "V2 slot zero must submit the first forward image on the anchor");
    expect(forward[1].submittedLogicalFrame == 1 && forward[3].submittedLogicalFrame == 2,
        "V2 forward submission must advance one refresh before visibility changes");

    const auto reverse = buildV2RefreshSubmissions(3, 2, true, &error);
    expect(reverse[0].visibleLogicalFrame == 2 && reverse[1].submittedLogicalFrame == 1
            && reverse[5].submittedLogicalFrame == 0,
        "V2 reverse mapping must mirror logical columns");
}

bool saveQueueFixture(const QString& path, int id)
{
    QImage image(1, 1, QImage::Format_RGB32);
    image.fill(qRgb(id, id, id));
    return image.save(path);
}

int frameId(const PrintFrame& frame)
{
    return frame.isValid() ? static_cast<unsigned char>(frame.pixels.at(0)) : -1;
}

QStringList nativeV2FileOrder(const QString& directory)
{
    QStringList files;
#ifdef Q_OS_WIN
    const QString pattern = QDir::toNativeSeparators(
        QDir(directory).absoluteFilePath(QStringLiteral("*.*")));
    WIN32_FIND_DATAW data {};
    HANDLE search = FindFirstFileW(
        reinterpret_cast<LPCWSTR>(pattern.utf16()), &data);
    expect(search != INVALID_HANDLE_VALUE, "native folder fixture must enumerate");
    do {
        const QString name = QString::fromWCharArray(data.cFileName);
        if (name != QStringLiteral(".") && name != QStringLiteral("..")
            && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            files.append(QDir(directory).absoluteFilePath(name));
        }
    } while (FindNextFileW(search, &data));
    FindClose(search);
#else
    Q_UNUSED(directory);
#endif
    return files;
}

void testFolderDiscoveryMatchesV2Win32Order()
{
#ifdef Q_OS_WIN
    QTemporaryDir root;
    expect(root.isValid(), "native-order fixture directory must exist");
    const QString unicodeFolder = QDir(root.path()).filePath(
        QString::fromUtf8("原生顺序"));
    expect(QDir().mkpath(unicodeFolder), "Unicode folder fixture must exist");
    expect(saveQueueFixture(QDir(unicodeFolder).filePath("z-19.png"), 19)
            && saveQueueFixture(QDir(unicodeFolder).filePath("a-41.png"), 41)
            && saveQueueFixture(QDir(unicodeFolder).filePath("m-8.png"), 8),
        "native-order folder fixtures must save");
    const QStringList expectedPaths = nativeV2FileOrder(unicodeFolder);
    QString error;
    const PrintImageSet source = loadPrintImagesFromFolder(unicodeFolder, &error);
    expect(error.isEmpty() && source.imageCount()
            == static_cast<size_t>(expectedPaths.size()), error);
    expect(saveQueueFixture(QDir(unicodeFolder).filePath("late-99.png"), 99),
        "late folder fixture must save");
    expect(source.imageCount() == static_cast<size_t>(expectedPaths.size()),
        "folder source list must remain frozen after discovery");
    std::unique_ptr<IPrintImageQueue> queue = source.createQueue();
    expect(queue && queue->start(&error), error);
    for (const QString& path : expectedPaths) {
        QVector<PrintFrame> row;
        expect(queue->takeRow(1, &row, &error), error);
        const QString base = QFileInfo(path).completeBaseName();
        const int expectedId = base.mid(base.indexOf('-') + 1).toInt();
        expect(row.size() == 1 && frameId(row.first()) == expectedId,
            "folder queue order must exactly match FindFirstFileW/FindNextFileW");
    }
    queue->stop();

    QFile ordinaryFile(QDir(unicodeFolder).filePath("ordinary-data.txt"));
    expect(ordinaryFile.open(QIODevice::WriteOnly), "ordinary file fixture must open");
    ordinaryFile.write("not an image");
    ordinaryFile.close();
    const PrintImageSet allFiles = loadPrintImagesFromFolder(unicodeFolder, &error);
    expect(error.isEmpty()
            && allFiles.imageCount() == source.imageCount() + 2,
        "V2 *.* discovery must retain every non-directory file without filtering");
#endif
}

void testFolderGridInfoUsesV2LastJpegFileName()
{
    QTemporaryDir root;
    expect(root.isValid(), "V2 grid-name fixture directory must exist");
    expect(saveQueueFixture(QDir(root.path()).filePath("001001.jpg"), 1)
            && saveQueueFixture(QDir(root.path()).filePath("002003.jpg"), 2),
        "V2 grid-name fixtures must save");
    QString error;
    const PrintImageFolderLoadResult loaded =
        loadPrintImagesFromFolderWithGridInfo(root.path(), &error);
    expect(error.isEmpty() && loaded.images.isValid(), error);
    expect(loaded.hasInferredGrid()
            && loaded.gridRows == 2 && loaded.gridColumns == 3,
        "folder grid must use V2's final RRRCCC.jpg file name");
}

void testRunnerUsesV2TruncatedFirstImageScaleForWholeRow()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    preflight.result.ok = true;
    PrintJobSnapshot job = makeJob(1, 2);
    job.config.main.widthScale = 1.5;
    job.config.main.heightScale = 1.5;
    PrintJobRunner runner(motion, exposure, presenter, preflight);
    QString error;
    expect(runner.start(job, &error), error);
    expect(presenter.lastPreparedTargetSizes
            == QVector<QSize>({QSize(1, 1), QSize(1, 1)}),
        "V2 cache must truncate first-image scaling and apply one target size to its whole row");
}

void testFolderAndMemoryQueuesYieldTheSameRowsInSourceOrder()
{
    QTemporaryDir root;
    expect(root.isValid(), "queue fixture directory must exist");
    const QVector<int> ids = {31, 12, 44, 7};
    QStringList explicitV2Order;
    QVector<PrintFrame> memoryFrames;
    for (int index = 0; index < ids.size(); ++index) {
        const QString path = QDir(root.path()).filePath(
            QString("native-slot-%1.png").arg(index));
        expect(saveQueueFixture(path, ids.at(index)), "folder queue fixture must save");
        explicitV2Order.append(path);
        memoryFrames.append(frameWithId(ids.at(index)));
    }

    QString error;
    const PrintImageSet folder = PrintImageSet::fromFolderFiles(
        explicitV2Order, root.path(), &error);
    expect(error.isEmpty(), error);
    const PrintImageSet memory = PrintImageSet::fromFrames(memoryFrames, &error,
        PrintImageSourceType::ElementalMemory);
    expect(error.isEmpty(), error);
    std::unique_ptr<IPrintImageQueue> folderQueue = folder.createQueue();
    std::unique_ptr<IPrintImageQueue> memoryQueue = memory.createQueue();
    expect(folderQueue && memoryQueue, "both source types must create the common queue");
    expect(folderQueue->start(&error), error);
    expect(memoryQueue->start(&error), error);

    for (int rowIndex = 0; rowIndex < 2; ++rowIndex) {
        QVector<PrintFrame> folderRow;
        QVector<PrintFrame> memoryRow;
        expect(folderQueue->takeRow(2, &folderRow, &error), error);
        expect(memoryQueue->takeRow(2, &memoryRow, &error), error);
        expect(folderRow.size() == 2 && memoryRow.size() == 2,
            "each queue must return a complete requested row");
        for (int column = 0; column < 2; ++column) {
            const int expectedId = ids.at(rowIndex * 2 + column);
            expect(frameId(folderRow.at(column)) == expectedId
                    && frameId(memoryRow.at(column)) == expectedId,
                "folder and memory queues must preserve the identical source order");
        }
    }
    folderQueue->stop();
    memoryQueue->stop();
}

void testQueueStartWaitsForV2PreloadTarget()
{
    QTemporaryDir root;
    expect(root.isValid(), "preload fixture directory must exist");
    const QString validPath = QDir(root.path()).filePath("first.png");
    expect(saveQueueFixture(validPath, 17), "preload fixture must save");
    QString error;
    const PrintImageSet source = PrintImageSet::fromFolderFiles(
        {validPath, QDir(root.path()).filePath("missing-second.png")},
        root.path(), &error);
    expect(error.isEmpty(), error);
    std::unique_ptr<IPrintImageQueue> queue = source.createQueue();
    expect(queue && !queue->start(&error) && !error.isEmpty(),
        "queue start must wait for the V2 preload target and surface producer failure");
    queue->stop();
}

void testFolderPreflightDoesNotDecodeBeforeQueueStart()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    motion.readiness = readyMotion();
    exposure.readiness = readyExposure();
    presenter.readiness = readyPresenter();
    PrintJobSnapshot job = makeJob(1, 2);
    QString error;
    job.images = PrintImageSet::fromFolderFiles(
        {QStringLiteral("missing-first.png"), QStringLiteral("missing-second.png")},
        QString(), &error);
    expect(error.isEmpty(), error);

    PrintHardwarePreflight preflight(motion, exposure, presenter);
    const PrintPreflightResult result = preflight.check(job, false);
    expect(result.ok,
        "folder preflight must retain grid/count checks without eager decoding");
}

void testRunnerTakesQueuedRowBeforePresentationWithoutSourceDecode()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    preflight.result.ok = true;
    QStringList trace;
    motion.trace = &trace;
    exposure.trace = &trace;
    presenter.trace = &trace;
    preflight.trace = &trace;
    auto probe = std::make_shared<QueueProbe>();
    probe->trace = &trace;
    probe->frames = {frameWithId(23), frameWithId(29)};
    PrintJobSnapshot job = makeJob(1, 2);
    QString error;
    job.images = PrintImageSet::fromFolderFiles(
        {QStringLiteral("missing-native-first"), QStringLiteral("missing-native-second")},
        QString(), &error);
    expect(error.isEmpty(), error);

    PrintJobRunner runner(motion, exposure, presenter, preflight,
        recordingQueueFactory(probe));
    expect(runner.start(job, &error), error);
    expect(probe->takeRowCalls == 1,
        "runner must request exactly one complete queued row");
    expect(trace.indexOf("queue_start") < trace.indexOf("queue_take_row")
            && trace.indexOf("queue_take_row") < trace.indexOf("present_anchor:23")
            && trace.indexOf("present_anchor:23") < trace.indexOf("prepare_row:23")
            && trace.indexOf("prepare_row:23") < trace.indexOf("prime:0")
            && trace.indexOf("prime:0") < trace.indexOf("vblank_anchor"),
        "queued row must be available before row presentation/prime and VBlank");
}

void testRunnerStopsQueueBeforeSafetyCleanupOnCancelAndFailure()
{
    const auto runCase = [](bool cancelDuringPresent) {
        RecordingMotion motion;
        RecordingExposure exposure;
        RecordingPresenter presenter;
        RecordingPreflight preflight;
        preflight.result.ok = true;
        QStringList trace;
        motion.trace = &trace;
        exposure.trace = &trace;
        presenter.trace = &trace;
        auto probe = std::make_shared<QueueProbe>();
        probe->trace = &trace;
        probe->frames = {frameWithId(3), frameWithId(4)};
        PrintJobSnapshot job = makeJob(1, 2);
        PrintJobRunner runner(motion, exposure, presenter, preflight,
            recordingQueueFactory(probe));
        if (cancelDuringPresent) {
            presenter.onPreparedPresent = [&runner] { runner.cancel(); };
        } else {
            presenter.prepareOk = false;
        }
        QString error;
        expect(!runner.start(job, &error),
            cancelDuringPresent ? "cancelled queue job must fail"
                                : "failed row preparation must fail");
        expect(probe->stopCalls == 1, "terminal path must stop/join queue once");
        expect(trace.indexOf("queue_stop_join") >= 0
                && trace.indexOf("queue_stop_join") < trace.indexOf("cleanup_stop")
                && trace.indexOf("queue_stop_join") < trace.indexOf("presenter_shutdown"),
            "queue stop/join must finish before safety cleanup");
    };

    runCase(true);
    runCase(false);
}

void testRunnerUsesStrictV2PreparedRowScheduling()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    preflight.result.ok = true;
    QStringList trace;
    std::mutex traceMutex;
    motion.trace = &trace;
    motion.traceMutex = &traceMutex;
    exposure.trace = &trace;
    exposure.traceMutex = &traceMutex;
    presenter.trace = &trace;
    presenter.traceMutex = &traceMutex;
    presenter.traceLegacyAnchor = false;

    PrintJobSnapshot job = makeJob(2, 2);
    job.plan.framesPerImage = 2;
    for (V2RowPlan& row : job.plan.rows) {
        row.startDelayFrames = 0;
        row.holdFramesAfterPresent = 1;
    }

    PrintJobRunner runner(motion, exposure, presenter, preflight);
    QObject::connect(&runner, &PrintJobRunner::rowDirectionChanged,
        [&exposure](bool reverse) { exposure.lastReverse = reverse; });

    QString error;
    expect(runner.start(job, &error), error);
    const QStringList expected = {
        "prepare_row:0", "prime:0", "vblank", "arm:forward", "prepare_y:forward", "cached_present:0", "vblank_anchor",
        "start_prepared_y", "cached_present:0", "vblank",
        "cached_present:1", "vblank", "cached_present:1", "vblank",
        "cached_present:1", "disarm", "wait_y", "move_x", "wait_x",
        "prepare_row:0", "arm:reverse", "prepare_y:reverse", "prime:1", "vblank_anchor",
        "start_prepared_y", "cached_present:1", "vblank",
        "cached_present:0", "vblank", "cached_present:0", "vblank",
        "cached_present:0", "disarm", "wait_y", "cleanup_stop",
        "cleanup_wait_stopped", "return_xy_zero", "presenter_shutdown"
    };
    expect(trace == expected,
        QString("V2 prepared-row runner trace mismatch\nactual: %1\nexpected: %2")
            .arg(trace.join(","), expected.join(",")));
    expect(presenter.prepareRowCalls == 2,
        "runner must prepare exactly one complete cache per row");
    expect(presenter.presentCalls == 0,
        "runner must not use frame-uploading present(frame) once scanning begins");
    expect(presenter.frameStatisticsSampleCalls == presenter.waitSlotCalls,
        "V2 must sample frame statistics once after every row-slot VBlank and before Present");
    expect(presenter.prepareRowThread == std::this_thread::get_id()
            && presenter.anchorThread == presenter.displayWorkerThread
            && presenter.displayWorkerThread != std::this_thread::get_id(),
        "V2 must prepare the row cache on the print thread before the paced display worker begins");
#ifdef Q_OS_WIN
    expect(presenter.displayWorkerPriority == THREAD_PRIORITY_HIGHEST,
        "the V2 display worker must run at THREAD_PRIORITY_HIGHEST");
#endif
}

void testDisplayWorkerPriorityFailureStopsBeforeD3DOrMotion()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    preflight.result.ok = true;
    std::atomic_int configureCalls {0};
    V2DisplayThreadConfigurer rejectPriority = [&](QString* errorMessage) {
        ++configureCalls;
        if (errorMessage)
            *errorMessage = "injected display priority failure";
        return false;
    };
    PrintJobRunner runner(motion, exposure, presenter, preflight,
        PrintImageQueueFactory(), std::shared_ptr<IPrintFlowLogger>(),
        rejectPriority);

    QString error;
    expect(!runner.start(makeJob(1, 2), &error),
        "display priority failure must fail the print job");
    expect(error.contains("injected display priority failure"),
        "display priority failure must remain actionable");
    expect(configureCalls.load() == 1,
        "each row display worker must configure its priority exactly once");
    expect(presenter.prepareRowCalls == 1
            && presenter.preparedPresentCalls == 1,
        "V2 must complete its one-time cache warm-up before the display-worker priority gate");
    expect(exposure.armCalls == 1 && motion.yMoves == 0,
        "V2's display worker is created after Y preparation but before Y motion starts");
}

void testRunnerStartsStrictV2SequenceWithoutStartDelayWait()
{
    const auto runCase = [](bool reverse) {
        RecordingMotion motion;
        RecordingExposure exposure;
        RecordingPresenter presenter;
        RecordingPreflight preflight;
        preflight.result.ok = true;
        QStringList trace;
        std::mutex traceMutex;
        motion.trace = &trace;
        motion.traceMutex = &traceMutex;
        exposure.trace = &trace;
        exposure.traceMutex = &traceMutex;
        presenter.trace = &trace;
        presenter.traceMutex = &traceMutex;
        presenter.traceLegacyAnchor = false;

        PrintJobSnapshot job = makeJob(1, 2);
        job.plan.framesPerImage = 2;
        V2RowPlan& row = job.plan.rows[0];
        if (reverse) {
            row = makeJob(2, 2).plan.rows.at(1);
        }
        row.reverse = reverse;
        row.logicalFrameOrder = reverse ? QVector<int>{1, 0} : QVector<int>{0, 1};
        row.startDelayFrames = 2;
        row.holdFramesAfterPresent = 1;
        PrintJobRunner runner(motion, exposure, presenter, preflight);
        QObject::connect(&runner, &PrintJobRunner::rowDirectionChanged,
            [&exposure](bool isReverse) { exposure.lastReverse = isReverse; });

        QString error;
        expect(runner.start(job, &error), error);
        const QStringList expectedCached = reverse
            ? QStringList{"cached_present:1", "cached_present:0",
                "cached_present:0", "cached_present:0"}
            : QStringList{"cached_present:0", "cached_present:1",
                "cached_present:1", "cached_present:1"};
        QStringList actualCached;
        for (const QString& event : trace) {
            if (event.startsWith("cached_present:")) actualCached.append(event);
        }
        // The first cached present is V2's one-time pre-row warm-up; the
        // strict sequence begins with the following cached submission.
        if (actualCached.size() == expectedCached.size() + 1) actualCached.removeFirst();
        expect(actualCached == expectedCached,
            QString("strict V2 scheduling must preserve the exact W*F cached refresh sequence; actual=%1 expected=%2")
                .arg(actualCached.join(','), expectedCached.join(',')));
        expect(presenter.waitDisplayCalls == 1,
            "strict V2 scheduling must use only its one pre-row warm-up VBlank before submission zero");
        expect(presenter.waitSlotCalls == expectedCached.size(),
            "only V2 refresh submissions may use row slots");

    };

    runCase(false);
    runCase(true);
}

void testRunnerWaitsForV2PositionGateBeforeStrictSequence()
{
    const auto runCase = [](bool reverse) {
        PositionGateMotion motion;
        RecordingExposure exposure;
        RecordingPresenter presenter;
        RecordingPreflight preflight;
        preflight.result.ok = true;
        QStringList trace;
        std::mutex traceMutex;
        motion.trace = &trace;
        motion.traceMutex = &traceMutex;
        presenter.trace = &trace;
        presenter.traceMutex = &traceMutex;
        presenter.traceLegacyAnchor = false;
        motion.plannedYPositions = reverse
            ? QVector<qint32>{100, 91, 90, 80}
            : QVector<qint32>{100, 109, 110, 120};

        PrintJobSnapshot job = makeJob(1, 2);
        job.plan.framesPerImage = 1;
        job.plan.stepPulse = 10;
        V2RowPlan& row = job.plan.rows[0];
        row.reverse = reverse;
        row.logicalFrameOrder = reverse ? QVector<int>{1, 0} : QVector<int>{0, 1};
        row.constantBegin = 100;
        row.startDelayFrames = 2;
        row.effectiveDisplayDelayPulse = 20;
        row.holdFramesAfterPresent = 0;
        row.yTarget = reverse ? 80 : 120;
        job.plan.accelerationPulse = 0;
        job.plan.presentPredictPulse = 10;

        const auto flowLogger = std::make_shared<RecordingFlowLogger>();
        PrintJobRunner runner(motion, exposure, presenter, preflight,
            PrintImageQueueFactory(), flowLogger);
        QString error;
        expect(runner.start(job, &error), error);

        const QStringList records = flowLogger->snapshot();
        const QString expectedGate = reverse
            ? QStringLiteral("observed_y_planned_pulse=90")
            : QStringLiteral("observed_y_planned_pulse=110");
        expect(std::any_of(records.cbegin(), records.cend(), [&](const QString& record) {
            return record.contains("event=v2_sequence_position_gate")
                && record.contains(expectedGate);
        }),
            "strict V2 submissions must use V2's one-refresh predictive Y-position gate");
        expect(std::any_of(records.cbegin(), records.cend(), [](const QString& record) {
            return record.contains("event=first_column_request")
                && record.contains("request_seq=1");
        }),
            "the V2 print thread must publish its first image through request sequence 1");
        expect(motion.firstTelemetrySampleThread == std::this_thread::get_id(),
            "V2's print thread, not the display worker, must own the Y-position selector");
    };

    runCase(false);
    runCase(true);
}

void testRunnerArmsExposureBeforeV2RowAnchor()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    preflight.result.ok = true;
    QStringList trace;
    std::mutex traceMutex;
    motion.trace = &trace;
    motion.traceMutex = &traceMutex;
    exposure.trace = &trace;
    exposure.traceMutex = &traceMutex;
    presenter.trace = &trace;
    presenter.traceMutex = &traceMutex;
    presenter.traceLegacyAnchor = false;

    PrintJobRunner runner(motion, exposure, presenter, preflight);
    QString error;
    expect(runner.start(makeJob(1, 2), &error), error);

    const int exposureArm = trace.indexOf(QStringLiteral("arm:forward"));
    const int vblankAnchor = trace.indexOf(QStringLiteral("vblank_anchor"));
    expect(exposureArm >= 0 && vblankAnchor >= 0 && exposureArm < vblankAnchor,
        QStringLiteral("V2 exposure-window arm must finish before the row VBlank anchor; "
                       "the anchor-to-start path may contain only the prepared Y-axis start."));
}

void testRunnerPreparesYBeforeV2RowAnchorAndStartsItAfterwards()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    preflight.result.ok = true;
    QStringList trace;
    std::mutex traceMutex;
    motion.trace = &trace;
    motion.traceMutex = &traceMutex;
    exposure.trace = &trace;
    exposure.traceMutex = &traceMutex;
    presenter.trace = &trace;
    presenter.traceMutex = &traceMutex;
    presenter.traceLegacyAnchor = false;

    PrintJobRunner runner(motion, exposure, presenter, preflight);
    QString error;
    expect(runner.start(makeJob(1, 2), &error), error);

    const int prepareY = trace.indexOf(QStringLiteral("prepare_y:forward"));
    const int vblankAnchor = trace.indexOf(QStringLiteral("vblank_anchor"));
    const int startY = trace.indexOf(QStringLiteral("start_prepared_y"));
    expect(prepareY >= 0 && vblankAnchor >= 0 && startY > vblankAnchor
            && prepareY < vblankAnchor,
        QStringLiteral("V2 must configure the Y target before the row VBlank anchor "
                       "and issue only the prepared-axis start after it."));
}

void testRunnerUsesV2RowVBlankCounterForTheWholeRow()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    preflight.result.ok = true;
    QStringList trace;
    std::mutex traceMutex;
    motion.trace = &trace;
    motion.traceMutex = &traceMutex;
    exposure.trace = &trace;
    exposure.traceMutex = &traceMutex;
    presenter.trace = &trace;
    presenter.traceMutex = &traceMutex;
    presenter.traceLegacyAnchor = false;
    presenter.recordRowVBlankCounter = true;

    PrintJobRunner runner(motion, exposure, presenter, preflight);
    QString error;
    expect(runner.start(makeJob(1, 2), &error), error);

    const int counterPrepare = trace.indexOf(QStringLiteral("vblank_counter_prepare"));
    const int anchor = trace.indexOf(QStringLiteral("vblank_anchor"));
    const int counterBegin = trace.indexOf(QStringLiteral("vblank_counter_begin"));
    const int startY = trace.indexOf(QStringLiteral("start_prepared_y"));
    const int counterStop = trace.indexOf(QStringLiteral("vblank_counter_stop"));
    const int disarm = trace.indexOf(QStringLiteral("disarm"));
    const int waitY = trace.indexOf(QStringLiteral("wait_y"));
    expect(counterPrepare >= 0 && counterPrepare < anchor && counterBegin > anchor
            && counterBegin < startY && counterStop > startY
            && counterStop < disarm && disarm < waitY,
        QStringLiteral("V2 RowVBlankCounter must prepare before the anchor, begin before "
                       "the prepared Y start, and stop when its display sequence completes."));
}

void testDisplayWorkerFailuresJoinBeforeSafetyCleanup()
{
    const auto runCase = [](bool failVBlank) {
        RecordingMotion motion;
        RecordingExposure exposure;
        RecordingPresenter presenter;
        RecordingPreflight preflight;
        preflight.result.ok = true;
        QStringList trace;
        std::mutex traceMutex;
        motion.trace = &trace;
        motion.traceMutex = &traceMutex;
        exposure.trace = &trace;
        exposure.traceMutex = &traceMutex;
        presenter.trace = &trace;
        presenter.traceMutex = &traceMutex;
        presenter.traceLegacyAnchor = false;
        if (failVBlank)
            presenter.waitSlotFailureCall = 1;
        else
            presenter.preparedPresentFailureCall = 2;

        bool disarmObservedJoinedWorker = false;
        bool shutdownObservedJoinedWorker = false;
        exposure.onDisarm = [&] {
            disarmObservedJoinedWorker =
                !presenter.displayWorkerActive.load(std::memory_order_acquire)
                && presenter.displayWorkerThread != std::this_thread::get_id();
        };
        presenter.onShutdown = [&] {
            shutdownObservedJoinedWorker =
                !presenter.displayWorkerActive.load(std::memory_order_acquire)
                && presenter.displayWorkerThread != std::this_thread::get_id();
        };

        PrintJobSnapshot job = makeJob(1, 2);
        job.plan.framesPerImage = 2;
        job.plan.rows[0].startDelayFrames = 0;
        job.plan.rows[0].holdFramesAfterPresent = 1;
        PrintJobRunner runner(motion, exposure, presenter, preflight);
        QObject::connect(&runner, &PrintJobRunner::rowDirectionChanged,
            [&exposure](bool reverse) { exposure.lastReverse = reverse; });

        QString error;
        expect(!runner.start(job, &error),
            failVBlank ? "injected display-worker VBlank failure must fail the job"
                       : "injected cached-present failure must fail the job");
        expect(disarmObservedJoinedWorker && shutdownObservedJoinedWorker,
            "display worker must be joined before disarm and presenter shutdown");
        const int disarm = trace.indexOf("disarm");
        expect(disarm >= 0 && disarm < trace.indexOf("cleanup_stop")
                && trace.indexOf("cleanup_stop") < trace.indexOf("presenter_shutdown"),
            "scheduler failure must enter the existing ordered safety cleanup");
        expect(presenter.presentCalls == 0,
            "display-worker failure path must never fall back to present(frame)");
    };

    runCase(true);
    runCase(false);
}

void testAsyncPrintFlowLoggerNeverWritesOnCallingThread()
{
    std::mutex sinkMutex;
    std::condition_variable sinkCondition;
    bool sinkEntered = false;
    bool releaseSink = false;
    std::thread::id sinkThread;
    const std::thread::id callerThread = std::this_thread::get_id();

    AsyncPrintFlowLogger logger([&](const QString&) {
        std::unique_lock<std::mutex> lock(sinkMutex);
        sinkThread = std::this_thread::get_id();
        sinkEntered = true;
        sinkCondition.notify_all();
        sinkCondition.wait(lock, [&] { return releaseSink; });
    });

    auto enqueue = std::async(std::launch::async, [&] {
        logger.log(QStringLiteral("event=probe"));
    });
    expect(enqueue.wait_for(std::chrono::milliseconds(250))
            == std::future_status::ready,
        "flow logger enqueue must return without waiting for the blocked sink");
    enqueue.get();

    {
        std::unique_lock<std::mutex> lock(sinkMutex);
        expect(sinkCondition.wait_for(lock, std::chrono::seconds(2),
                   [&] { return sinkEntered; }),
            "background print-flow sink must receive the queued record");
        releaseSink = true;
    }
    sinkCondition.notify_all();
    expect(logger.flush(2000), "flow logger must flush queued records");
    expect(sinkThread != callerThread,
        "print-flow sink must never execute on the caller/VBlank thread");
}

void testRunnerLogsV2EquivalentFlowEvents()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    preflight.result.ok = true;
    auto logger = std::make_shared<RecordingFlowLogger>();
    PrintJobSnapshot job = makeJob(1, 2);
    job.plan.framesPerImage = 2;
    job.plan.rows[0].startDelayFrames = 1;
    job.plan.rows[0].holdFramesAfterPresent = 1;

    PrintJobRunner runner(motion, exposure, presenter, preflight,
        PrintImageQueueFactory(), logger);
    QString error;
    expect(runner.start(job, &error), error);

    const QString joined = logger->snapshot().join('\n');
    const QStringList requiredRecords = {
        QStringLiteral("event=dxgi_row_anchor row=1 dir=forward prime_logical_pic=0"),
        QStringLiteral("event=vsync_display_config row=1 dir=forward"),
        QStringLiteral("frames_per_image=2"),
        QStringLiteral("startDelayFrames=1"),
        QStringLiteral("hold_frames_after_present=1"),
        QStringLiteral("event=motion_y_start row=1 dir=forward"),
        QStringLiteral("event=motion_y_stop row=1"),
        QStringLiteral("event=exposure_on row=1 dir=forward"),
        QStringLiteral("event=exposure_off row=1"),
        QStringLiteral("event=dxgi_present_submit row=1 dir=forward refresh_slot=0"),
        QStringLiteral("visible_col=0"),
        QStringLiteral("visible_logical_pic=0"),
        QStringLiteral("submit_logical_pic=1"),
        QStringLiteral("event=dxgi_present_axis_relation row=1 dir=forward refresh_slot=0"),
        QStringLiteral("axis_sample_available=1"),
        QStringLiteral("vblank_method=row_prime"),
        QStringLiteral("row_anchor_source=swap_chain_output_vblank"),
        QStringLiteral("event=run_end reason=completed total_rows=1")
    };
    for (const QString& token : requiredRecords) {
        expect(joined.contains(token),
            QString("V2 print-flow record must contain '%1'\nrecords:\n%2")
                .arg(token, joined));
    }

    const QVector<std::thread::id> writerThreads = logger->threadSnapshot();
    expect(std::any_of(writerThreads.cbegin(), writerThreads.cend(),
               [](const std::thread::id& id) {
                   return id != std::this_thread::get_id();
               }),
        "DXGI submission flow records must be emitted by the display worker");
}

void testRunnerLogsAbortBeforeFailedRunEnd()
{
    RecordingMotion motion;
    RecordingExposure exposure;
    RecordingPresenter presenter;
    RecordingPreflight preflight;
    preflight.result.ok = true;
    presenter.waitSlotFailureCall = 0;
    auto logger = std::make_shared<RecordingFlowLogger>();
    PrintJobSnapshot job = makeJob(1, 2);
    PrintJobRunner runner(motion, exposure, presenter, preflight,
        PrintImageQueueFactory(), logger);

    QString error;
    expect(!runner.start(job, &error), "injected VBlank failure must fail");
    const QString joined = logger->snapshot().join('\n');
    const int abortIndex = joined.indexOf(QStringLiteral(
        "event=run_abort reason=operation_failed row=1"));
    const int endIndex = joined.indexOf(QStringLiteral(
        "event=run_end reason=failed total_rows=1"));
    expect(abortIndex >= 0 && endIndex > abortIndex,
        QString("failed print must log run_abort before run_end\nrecords:\n%1")
            .arg(joined));
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    testPreflightMatrixAndZeroUnsafeCalls();
    testPreflightDoesNotAddV2ImageCountGate();
    testExactOneRowOrder();
    testExactOneRowAndTwoByThreeOrder();
    testV2SourceCountAndFrameValidity();
    testPreflightRejectsMalformedRowAddressing();
    testInvalidTargetScalesFailBeforeRounding();
    testPauseResumeRechecksAndDoesNotDuplicateFrames();
    testCancelDuringPresentDoesNotAdvanceAnotherVBlank();
    testCancelledCleanupFailureStillResetsProgress();
    testPausedCancelIsDrainedOnlyOnOwnerThread();
    testCleanupAggregatesFailuresAndStillShutsPresenterDown();
    testEveryEngineOperationFailureUsesCommonCleanup();
    testEveryTerminalCleanupFailureIsUnsafe();
    testIllegalRepeatedCommandsAndDestructionAreBounded();
    testPrintPositionSamplingAndConversion();
    testV2RefreshSubmissionTrace();
    testFolderDiscoveryMatchesV2Win32Order();
    testFolderGridInfoUsesV2LastJpegFileName();
    testRunnerUsesV2TruncatedFirstImageScaleForWholeRow();
    testFolderAndMemoryQueuesYieldTheSameRowsInSourceOrder();
    testQueueStartWaitsForV2PreloadTarget();
    testFolderPreflightDoesNotDecodeBeforeQueueStart();
    testRunnerTakesQueuedRowBeforePresentationWithoutSourceDecode();
    testRunnerStopsQueueBeforeSafetyCleanupOnCancelAndFailure();
    testRunnerUsesStrictV2PreparedRowScheduling();
    testDisplayWorkerPriorityFailureStopsBeforeD3DOrMotion();
    testRunnerStartsStrictV2SequenceWithoutStartDelayWait();
    testRunnerWaitsForV2PositionGateBeforeStrictSequence();
    testRunnerArmsExposureBeforeV2RowAnchor();
    testRunnerPreparesYBeforeV2RowAnchorAndStartsItAfterwards();
    testRunnerUsesV2RowVBlankCounterForTheWholeRow();
    testDisplayWorkerFailuresJoinBeforeSafetyCleanup();
    testAsyncPrintFlowLoggerNeverWritesOnCallingThread();
    testRunnerLogsV2EquivalentFlowEvents();
    testRunnerLogsAbortBeforeFailedRunEnd();
    qInfo() << "V2 print engine core tests passed";
    return 0;
}

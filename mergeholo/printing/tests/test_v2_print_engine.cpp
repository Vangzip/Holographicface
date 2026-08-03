#include "../PrintHardwarePreflight.h"
#include "../PrintJobRunner.h"
#include "../PrintPositionSampler.h"

#include <QCoreApplication>
#include <QDebug>

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <functional>
#include <limits>
#include <thread>

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

class RecordingMotion final : public IMotionController {
public:
    PrintMotionReadiness readiness;
    QStringList* trace = nullptr;
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
    bool yWaitOk = true;
    bool xMoveOk = true;
    bool xWaitOk = true;
    std::function<void()> onWaitY;

    PrintMotionReadiness printReadiness(QString*) const override { return readiness; }
    bool beginPrint(QString*) override { return beginOk; }
    void endPrint() override {}
    bool startYScan(qint32 target, const PrintAxisConfig&, QString*) override
    {
        ++yMoves;
        if (trace) trace->append(target > 0 ? "move_y:forward" : "move_y:reverse");
        return yMoveOk;
    }
    bool waitYStopped(int, const std::atomic_bool&, QString*) override
    {
        if (trace) trace->append("wait_y");
        if (onWaitY) onWaitY();
        return yWaitOk;
    }
    bool stepX(qint32, const PrintAxisConfig&, QString*) override
    {
        ++xMoves;
        if (trace) trace->append("move_x");
        return xMoveOk;
    }
    bool waitXStopped(int, const std::atomic_bool&, QString*) override
    {
        if (trace) trace->append("wait_x");
        return xWaitOk;
    }
    bool stopMappedAxes(QString*) override
    {
        ++cleanupStops;
        if (trace) trace->append("cleanup_stop");
        return stopOk;
    }
    bool waitMappedAxesStopped(int, QString*) override { return waitMappedOk; }
    bool returnToLogicalZero(const PrintAxisConfig&, const PrintAxisConfig&, int, QString*) override
    {
        ++returnZeroCalls;
        if (trace) trace->append("return_xy_zero");
        return returnZeroOk;
    }
    bool verifyLogicalZero(QString*) override { return verifyZeroOk; }
};

class RecordingExposure final : public IExposureController {
public:
    PrintExposureReadiness readiness;
    QStringList* trace = nullptr;
    int armCalls = 0;
    int disarmCalls = 0;
    bool disarmOk = true;
    bool armOk = true;

    PrintExposureReadiness printReadiness(QString*) const override { return readiness; }
    bool arm(qint32, qint32, QString*) override
    {
        ++armCalls;
        if (trace) trace->append(lastReverse ? "arm:reverse" : "arm:forward");
        return armOk;
    }
    bool disarm(QString*) override
    {
        ++disarmCalls;
        if (trace && armCalls > 0 && disarmCalls <= armCalls) trace->append("disarm");
        return disarmOk;
    }
    bool isArmed() const override { return false; }
    bool lastReverse = false;
};

class RecordingPresenter final : public IPrintFramePresenter {
public:
    PrintPresenterReadiness readiness;
    QStringList* trace = nullptr;
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
    std::function<void()> onPresent;

    PrintPresenterReadiness printReadiness(QString*) const override { return readiness; }
    bool prepare(const PrintFrame& frame, const QSize&, QString*) override
    {
        ++prepareCalls;
        if (trace) trace->append("present_anchor:" + QString::number(static_cast<unsigned char>(frame.pixels[0])));
        return prepareOk;
    }
    bool presentRowAnchor(const PrintFrame& frame, const QSize&, QString*) override
    {
        if (trace) trace->append("present_anchor:" + QString::number(static_cast<unsigned char>(frame.pixels[0])));
        return rowAnchorOk;
    }
    bool present(const PrintFrame&, const QSize&, QString*) override
    {
        ++presentCalls;
        if (onPresent) onPresent();
        return presentOk;
    }
    bool waitForDisplayFrame(QString*) override { return waitDisplayOk; }
    bool acquireRowAnchor(PrintRowVBlankAnchor* anchor, QString*) override
    {
        anchor->available = true;
        if (trace) trace->append("vblank_anchor");
        return acquireOk;
    }
    bool waitForRowSlot(int, PrintRowVBlankAnchor*, QString*) override
    {
        if (trace) trace->append("vblank");
        return waitSlotOk;
    }
    void shutdown() override
    {
        ++shutdownCalls;
        if (trace) trace->append("presenter_shutdown");
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
        {PreflightFault::ImageCount, "image"},
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
        case PreflightFault::ImageCount: job.config.main.gridColumns = 2; break;
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
        "preflight", "present_anchor:0", "vblank_anchor", "arm:forward",
        "move_y:forward", "frame:0", "vblank", "frame:1", "vblank",
        "frame:2", "vblank", "wait_y", "disarm", "cleanup_stop",
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
        "present_anchor:0", "vblank_anchor", "arm:forward", "move_y:forward",
        "frame:0", "vblank", "frame:1", "vblank", "frame:2", "vblank",
        "wait_y", "disarm", "move_x", "wait_x",
        "present_anchor:2", "vblank_anchor", "arm:reverse", "move_y:reverse",
        "frame:2", "vblank", "frame:1", "vblank", "frame:0", "vblank",
        "wait_y", "disarm", "cleanup_stop", "return_xy_zero",
        "presenter_shutdown", "finished_success"
    };
    expect(trace == expected,
        QString("2x3 event order mismatch\nactual: %1\nexpected: %2")
            .arg(trace.join(","), expected.join(",")));
    expect(runner.state() == PrintJobState::Ready, "successful job must return Ready");
}

void testExactSourceCountAndFrameValidity()
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
    expect(!preflight.check(job, false).ok, "five frames must not satisfy a 2x3 plan");

    QVector<PrintFrame> extraFrames;
    for (int i = 0; i < 7; ++i) extraFrames.append(frameWithId(i));
    job.images = PrintImageSet::fromFrames(extraFrames, &error);
    expect(!preflight.check(job, false).ok, "seven frames must not satisfy a 2x3 plan");

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
    PrintJobRunner runner(motion, exposure, presenter, preflight);
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

    expect(runner.resume(&error), error);
    expect(trace.count("preflight_dynamic") == 1, "resume must rerun dynamic preflight");
    expect(trace.count("row1_frame:2") == 1 && trace.count("row1_frame:1") == 1
            && trace.count("row1_frame:0") == 1,
        "resume must neither duplicate nor skip reverse row");
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
    presenter.onPresent = [&] {
        std::thread caller([&] { runner.cancel(); });
        caller.join();
    };

    QString error;
    expect(!runner.start(makeJob(1, 3), &error), "cancelled job must not report success");
    const int firstRegularPresent = trace.indexOf("frame:1");
    expect(firstRegularPresent < 0,
        "cancel accepted inside Present must not publish the cancelled frame");
    expect(trace.count("vblank") == 1,
        "cancel accepted inside Present must not wait one more physical VBlank");
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
    presenter.onPresent = [&] { runner.cancel(); };

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
        {"frame Present", [](auto&, auto&, auto& p, auto&) { p.presentOk = false; }},
        {"y wait", [](auto& m, auto&, auto&, auto&) { m.yWaitOk = false; }},
        {"x move", [](auto& m, auto&, auto&, auto&) { m.xMoveOk = false; }},
        {"x wait", [](auto& m, auto&, auto&, auto&) { m.xWaitOk = false; }},
        {"next-row anchor", [](auto&, auto&, auto& p, auto&) { p.rowAnchorOk = false; }}
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

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    testPreflightMatrixAndZeroUnsafeCalls();
    testExactOneRowOrder();
    testExactOneRowAndTwoByThreeOrder();
    testExactSourceCountAndFrameValidity();
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
    qInfo() << "V2 print engine core tests passed";
    return 0;
}

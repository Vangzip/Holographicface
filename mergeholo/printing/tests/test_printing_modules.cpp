#include "PrintConfig.h"
#include "PrintHardwareProfile.h"
#include "DfjzhMotionController.h"
#include "LegacyPrintTiming.h"
#include "IPrintFramePresenter.h"
#include "IMotionController.h"
#include "PrintFrame.h"
#include "PrintImageSource.h"
#include "PrintJobRunner.h"
#include "SecondScreenSelection.h"
#include "elemental/ElementalMemoryResult.h"

#include <QCoreApplication>
#include <QColor>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QSettings>
#include <QTimer>
#include <QTemporaryDir>
#include <QVector>

#include <climits>
#include <array>
#include <functional>
#include <cstring>
#include <iostream>

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "check failed: " << message << "\n";
        std::exit(1);
    }
}

ElementalMemoryResult makeMemoryResult(size_t imageCount = 2)
{
    ElementalMemoryResult result;
    result.imageCount = imageCount;
    result.rows = 2;
    result.cols = 3;
    result.imageBytes = static_cast<size_t>(result.rows) * result.cols * 3;
    result.totalBytes = result.imageBytes * result.imageCount;
    result.targetRows = 1;
    result.targetCols = 2;
    result.mode = ElementalMemoryMode::Materialized;
    result.pixels.reset(new unsigned char[result.totalBytes]);
    for (size_t index = 0; index < result.totalBytes; ++index) {
        result.pixels[index] = static_cast<unsigned char>(index % 251);
    }
    return result;
}

struct RecordingMotionState {
    bool initializeResult = true;
    QString initializeError = "Motion card initialization failed.";
    QStringList events;
    QVector<long> yPositions;
    mutable int yPositionIndex = 0;
    std::array<long, 4> targets = { 0, 0, 0, 0 };
};

class RecordingMotionController final : public IMotionController
{
public:
    explicit RecordingMotionController(std::shared_ptr<RecordingMotionState> state)
        : state_(std::move(state))
    {
    }

    bool initialize(QString* errorMessage) override
    {
        state_->events << "initialize";
        if (!state_->initializeResult && errorMessage) {
            *errorMessage = state_->initializeError;
        }
        return state_->initializeResult;
    }

    void shutdown() override
    {
        state_->events << "shutdown";
    }

    bool setCurrentPositionAsOrigin(int axis, QString*) override
    {
        state_->events << QString("origin:%1").arg(axis);
        return true;
    }

    bool moveTo(int axis, long targetPulse, const PrintAxisConfig&, QString*) override
    {
        state_->targets[axis] = targetPulse;
        state_->events << QString("move:%1:%2").arg(axis).arg(targetPulse);
        return true;
    }

    bool stopAxis(int axis, QString*) override
    {
        state_->events << QString("stop:%1").arg(axis);
        return true;
    }

    bool waitUntilStopped(int axis, int, const std::atomic_bool&, QString*) override
    {
        state_->events << QString("wait:%1").arg(axis);
        return true;
    }

    long readPosition(int axis) const override
    {
        if (axis == 1 && !state_->yPositions.isEmpty()) {
            const int index = std::min(state_->yPositionIndex++, state_->yPositions.size() - 1);
            return state_->yPositions.at(index);
        }
        return state_->targets[axis];
    }

    bool setExposureOutput(bool enabled, int, QString*) override
    {
        state_->events << (enabled ? "o1:1" : "o1:0");
        return true;
    }

    bool armExposureWindow(long beginPos, long endPos, QString*) override
    {
        state_->events << QString("arm:%1:%2").arg(beginPos).arg(endPos);
        return true;
    }

    void disarmExposureWindow() override
    {
        state_->events << "disarm";
    }

private:
    std::shared_ptr<RecordingMotionState> state_;
};

class RecordingFramePresenter final : public IPrintFramePresenter
{
public:
    bool prepareResult = true;
    bool vBlankResult = true;
    QStringList events;
    std::function<void()> onPresent;

    bool prepare(const PrintFrame&, const QSize&, QString* errorMessage) override
    {
        events << "prepare";
        if (!prepareResult && errorMessage) {
            *errorMessage = "Second-screen prepare failed.";
        }
        return prepareResult;
    }

    bool present(const PrintFrame& frame, const QSize&, QString*) override
    {
        const int frameIndex = static_cast<unsigned char>(frame.pixels.at(0)) / 18;
        events << QString("present:%1").arg(frameIndex);
        if (onPresent) {
            onPresent();
        }
        return true;
    }

    bool waitForVBlank(QString* errorMessage) override
    {
        events << "vblank";
        if (!vBlankResult && errorMessage) {
            *errorMessage = "Second-screen VSync failed.";
        }
        return vBlankResult;
    }

    void shutdown() override
    {
        events << "presenter_shutdown";
    }
};

struct JobResult {
    bool success = false;
    QString text;
};

JobResult runJobAndWait(
    PrintJobRunner& runner,
    const Print9030Config& config,
    const PrintImageSet& images,
    const std::shared_ptr<IPrintFramePresenter>& presenter)
{
    QEventLoop loop;
    JobResult result;
    bool timedOut = false;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&] {
        timedOut = true;
        loop.quit();
    });
    QObject::connect(&runner, &PrintJobRunner::finished, &loop, [&](bool success, const QString& text) {
        result = { success, text };
        loop.quit();
    });

    QString error;
    expect(runner.start(config, images, presenter, &error), "runner should start");
    timeout.start(5000);
    loop.exec();
    expect(!timedOut, "runner should finish within the test timeout");
    return result;
}

int eventIndex(const QStringList& events, const QString& event)
{
    return events.indexOf(event);
}

int eventCount(const QStringList& events, const QString& event)
{
    return events.count(event);
}

void testElementalMemoryIsPreferredPrintSource()
{
    ElementalMemoryResult memory = makeMemoryResult();
    QString error;
    PrintImageSet set = makePrintImageSetFromElementalMemory(memory, &error);
    expect(error.isEmpty(), "elemental memory source should not set an error");
    expect(set.sourceType == PrintImageSourceType::ElementalMemory, "source type should be memory");
    expect(set.imageCount() == 2, "memory image count should match elemental result");
    expect(set.imageWidth == 3, "memory image width should match elemental cols");
    expect(set.imageHeight == 2, "memory image height should match elemental rows");

    QByteArray copied;
    expect(set.copyImageBytes(1, &copied), "copyImageBytes should copy memory image");
    expect(static_cast<size_t>(copied.size()) == memory.imageBytes, "copied image size should match");
    expect(std::memcmp(copied.constData(), memory.pixels.get() + memory.imageBytes, memory.imageBytes) == 0,
        "copied image bytes should match source");
}

void testElementalMemoryCopiesBgrPrintFrame()
{
    ElementalMemoryResult memory = makeMemoryResult();
    QString error;
    PrintImageSet set = makePrintImageSetFromElementalMemory(memory, &error);

    PrintFrame frame;
    expect(set.copyFrame(1, &frame, &error), "elemental memory should copy a display frame");
    expect(error.isEmpty(), "elemental frame copy should not set an error");
    expect(frame.isValid(), "elemental display frame should be valid");
    expect(frame.width == 3 && frame.height == 2, "elemental frame dimensions should match source");
    expect(frame.format == PrintPixelFormat::Bgr24, "elemental memory should remain BGR24");
    expect(static_cast<size_t>(frame.pixels.size()) == memory.imageBytes, "elemental frame byte count should match");
    const unsigned char* source = memory.pixels.get() + memory.imageBytes;
    expect(static_cast<unsigned char>(frame.pixels.at(0)) == source[2]
            && static_cast<unsigned char>(frame.pixels.at(1)) == source[1]
            && static_cast<unsigned char>(frame.pixels.at(2)) == source[0],
        "elemental RGB pixels should convert to the legacy BGR display order");
}

void testManualFolderFallbackEnumeratesImages()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temporary directory should be valid");
    const QString jpgPath = QDir(dir.path()).filePath("002.jpg");
    const QString pngPath = QDir(dir.path()).filePath("001.png");
    QImage image(2, 1, QImage::Format_RGB888);
    image.fill(QColor(0x12, 0x34, 0x56));
    expect(image.save(jpgPath), "temporary JPG should be written");
    expect(image.save(pngPath), "temporary PNG should be written");
    QFile(QDir(dir.path()).filePath("notes.txt")).open(QIODevice::WriteOnly);

    QString error;
    PrintImageSet set = loadPrintImagesFromFolder(dir.path(), &error);
    expect(error.isEmpty(), "folder source should not set an error");
    expect(set.sourceType == PrintImageSourceType::Folder, "source type should be folder");
    expect(set.imageCount() == 2, "folder image count should ignore non-images");
    expect(set.files.at(0).endsWith("001.png"), "folder files should be sorted by name");
    expect(set.files.at(1).endsWith("002.jpg"), "folder files should be sorted by name");

    PrintFrame frame;
    expect(set.copyFrame(0, &frame, &error), "folder image should decode into a display frame");
    expect(error.isEmpty(), "folder frame decode should not set an error");
    expect(frame.isValid(), "folder display frame should be valid");
    expect(frame.width == 2 && frame.height == 1, "folder frame dimensions should match decoded image");
    expect(frame.format == PrintPixelFormat::Bgr24, "folder image should convert to BGR24");
    expect(static_cast<unsigned char>(frame.pixels.at(0)) == 0x56, "folder frame should use BGR channel order");
    expect(static_cast<unsigned char>(frame.pixels.at(1)) == 0x34, "folder frame green channel should be preserved");
    expect(static_cast<unsigned char>(frame.pixels.at(2)) == 0x12, "folder frame red channel should be preserved");
}

void testImc60gProductionProfileMatchesV2()
{
    QString error;
    const PrintHardwareProfile p = loadPrintHardwareProfile(
        QDir::current().absoluteFilePath("../../../config/imc60g_print.ini"), &error);
    expect(error.isEmpty(), "hardware profile should load");
    expect(p.version == 1 && p.cardIndex == 0, "card profile should be versioned");
    expect(p.axisX == 1 && p.axisY == 0, "X/Y mapping must match V2");
    expect(p.homeOrder == QVector<PrintHardwareProfile::LogicalAxis>({
        PrintHardwareProfile::LogicalAxis::Y,
        PrintHardwareProfile::LogicalAxis::X
    }), "home order must be logical Y then logical X");
    expect(p.homeDirectionX == -1 && p.homeDirectionY == -1, "home direction must match V2");
    expect(p.homeBackoffX == 28000 && p.homeBackoffY == 92000, "backoff pulses must match current V2 source");
    expect(p.printStepPulse == 1000 && p.forwardDelayPulse == 4000, "IMC print basis must match V2");
    expect(p.reverseFixedPulse == 2000 && p.exposureOffsetPulse == 2000, "IMC timing constants must match V2");
    expect(p.sv660nDoFunction == 25 && p.sv660nPointIndex == 1, "SV660N DO1 mapping must match V2");
    expect(p.sv660nPositiveAttribute == 129 && p.sv660nNegativeAttribute == 130,
        "SV660N crossing attributes must match V2");
}

void testPrintConfigDefaultsMatchV2()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temporary directory should be valid");
    const QString path = QDir(dir.path()).filePath("print_9030.ini");
    QString error;
    Print9030Config config = loadPrint9030Config(path, &error);
    expect(error.isEmpty(), "missing config should load defaults without error");
    expect(config.main.gridColumns == 150, "default columns should be 150");
    expect(config.main.gridRows == 150, "default rows should be 150");
    expect(config.axisX.subdivision == 40 && config.axisX.resolution == 50, "X must use 2000 units/mm");
    expect(config.axisX.speedOfMovement == 5000, "X speed must match V2 aixs_config.cfg");
    expect(config.axisY.speedOfMovement == 60000, "Y speed must match V2 aixs_config.cfg");
    expect(config.main.moveAdjustMm == 20.0, "move adjustment must match V2 main_config.cfg");
    expect(config.main.widthScale == 3.8 && config.main.heightScale == 2.8, "scale must match V2");
    expect(config.main.addTempPulse == 16000 && config.main.leadPulse == 1000, "IMC pulse defaults must match V2");

    config.main.gridColumns = 12;
    expect(savePrint9030Config(path, config, &error), "saving config should succeed");
    Print9030Config loaded = loadPrint9030Config(path, &error);
    expect(loaded.main.gridColumns == 12, "saved columns should round-trip");
}

void testPrintConfigMigratesEditableValuesWithoutLegacyShutter()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temporary directory should be valid");
    const QString path = QDir(dir.path()).filePath("legacy_print.ini");
    QFile file(path);
    expect(file.open(QIODevice::WriteOnly | QIODevice::Text), "legacy config should be writable");
    file.write("[main]\nmove_adjust_mm=17\ngrid_columns=12\n"
               "[axis_w]\nelectrical_status=true\n");
    file.close();

    QString error;
    const Print9030Config config = loadPrint9030Config(path, &error);
    expect(error.isEmpty(), "legacy config migration should not set an error");
    expect(config.configVersion == 2, "legacy config should migrate to version 2");
    expect(config.main.moveAdjustMm == 17.0 && config.main.gridColumns == 12,
        "legacy editable values should be retained");
    expect(!config.axisW.electricalStatus, "legacy W electrical status must not become a simulated shutter");

    expect(savePrint9030Config(path, config, &error), "migrated config should save");
    QSettings saved(path, QSettings::IniFormat);
    expect(saved.value("meta/version").toInt() == 2, "saved config should declare version 2");
}

void testMotionControllerRejectsUnavailableCard()
{
    const QString absentDll = QDir::temp().filePath("missing-9030-motion-card.dll");
    QFile::remove(absentDll);

    DfjzhMotionController controller(absentDll);
    QString error;
    expect(!controller.initialize(&error), "unavailable motion card DLL must fail initialization");
    expect(!error.isEmpty(), "motion card initialization failure should describe the problem");
    expect(!controller.setCurrentPositionAsOrigin(0, &error),
        "motion API must reject origin reset before card initialization");
}

void testSecondScreenSelectionRequiresNonPrimaryMonitor()
{
    const QVector<LegacyDisplayMonitor> displays = {
        { QRect(0, 0, 1920, 1080), true },
        { QRect(1920, 0, 1920, 1080), false },
        { QRect(-1280, 0, 1280, 1024), false }
    };
    const std::optional<int> selected = selectLegacySecondScreenIndex(displays);
    expect(selected.has_value(), "a non-primary display should be selected");
    expect(*selected == 2, "legacy selection should use the last non-primary display");

    const QVector<LegacyDisplayMonitor> primaryOnly = {
        { QRect(0, 0, 1920, 1080), true }
    };
    expect(!selectLegacySecondScreenIndex(primaryOnly).has_value(),
        "printing must not fall back to the primary display");
}

void testLegacyTimingMatchesDefault9030Scan()
{
    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 2;
    config.main.gridColumns = 3;

    LegacyPrintTiming timing;
    QString error;
    expect(calculateLegacyPrintTiming(config, &timing, &error), "default timing should be valid");
    expect(timing.yStepPulse == 1000, "Y step should match V2 pulses");
    expect(timing.accelerationPulse == 12000, "V2 acceleration pulse should match");
    expect(timing.framesPerImage == 1, "default scan must fit one 60 Hz frame per image");
    expect(timing.totalPulse == 43000, "V2 row travel should include acceleration and temporary pulse");

    const LegacyRowPlan forward = makeLegacyRowPlan(timing, 0, false);
    expect(forward.compareBegin == 12000 && forward.compareEnd == LONG_MAX,
        "forward compare window should match V2");
    expect(forward.yTarget == 43000, "forward target should match V2 row travel");

    const LegacyRowPlan reverse = makeLegacyRowPlan(timing, forward.yTarget, true);
    expect(reverse.compareBegin == LONG_MIN && reverse.compareEnd == 16000,
        "reverse compare window should match V2");
    expect(reverse.yTarget == 0, "reverse target should return to the logical origin");
}

void testLegacyTimingRejectsVBlankMismatch()
{
    Print9030Config config = defaultPrint9030Config();
    config.main.gridColumns = 3;
    config.axisY.speedOfMovement = 45000;

    LegacyPrintTiming timing;
    QString error;
    expect(!calculateLegacyPrintTiming(config, &timing, &error),
        "scan speed that does not fit a whole VBlank count must be rejected");
    expect(error.contains("VBlank"), "frame-fit failure should identify VBlank timing");
}

void testSecondScreenFailureDoesNotInitializeCard()
{
    ElementalMemoryResult memory = makeMemoryResult(1);
    QString error;
    const PrintImageSet images = makePrintImageSetFromElementalMemory(memory, &error);
    auto presenter = std::make_shared<RecordingFramePresenter>();
    presenter->prepareResult = false;
    auto motion = std::make_shared<RecordingMotionState>();
    PrintJobRunner runner([motion] { return std::make_unique<RecordingMotionController>(motion); });

    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 1;
    config.main.gridColumns = 1;
    const JobResult result = runJobAndWait(runner, config, images, presenter);

    expect(!result.success, "second-screen preflight failure should fail the job");
    expect(eventIndex(motion->events, "initialize") < 0, "second-screen failure must not initialize the card");
    expect(eventIndex(motion->events, "move:1:43000") < 0, "second-screen failure must not move an axis");
}

void testVBlankFailureDoesNotInitializeCard()
{
    ElementalMemoryResult memory = makeMemoryResult(1);
    QString error;
    const PrintImageSet images = makePrintImageSetFromElementalMemory(memory, &error);
    auto presenter = std::make_shared<RecordingFramePresenter>();
    presenter->vBlankResult = false;
    auto motion = std::make_shared<RecordingMotionState>();
    PrintJobRunner runner([motion] { return std::make_unique<RecordingMotionController>(motion); });

    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 1;
    config.main.gridColumns = 1;
    const JobResult result = runJobAndWait(runner, config, images, presenter);

    expect(!result.success, "VSync preflight failure should fail the job");
    expect(eventIndex(motion->events, "initialize") < 0, "VSync failure must not initialize the card");
}

void testCardInitializationFailureFinishesWithoutMotion()
{
    ElementalMemoryResult memory = makeMemoryResult(1);
    QString error;
    const PrintImageSet images = makePrintImageSetFromElementalMemory(memory, &error);
    auto presenter = std::make_shared<RecordingFramePresenter>();
    auto motion = std::make_shared<RecordingMotionState>();
    motion->initializeResult = false;
    PrintJobRunner runner([motion] { return std::make_unique<RecordingMotionController>(motion); });

    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 1;
    config.main.gridColumns = 1;
    const JobResult result = runJobAndWait(runner, config, images, presenter);

    expect(!result.success, "card initialization failure should fail the job");
    expect(eventIndex(motion->events, "initialize") >= 0, "card initialization should be attempted");
    expect(eventIndex(motion->events, "origin:0") < 0, "failed card initialization must not reset origins");
    expect(eventIndex(motion->events, "move:1:43000") < 0, "failed card initialization must not move an axis");
}

void testRunnerExecutesOneRowAndCleansUp()
{
    ElementalMemoryResult memory = makeMemoryResult(3);
    QString error;
    const PrintImageSet images = makePrintImageSetFromElementalMemory(memory, &error);
    auto presenter = std::make_shared<RecordingFramePresenter>();
    auto motion = std::make_shared<RecordingMotionState>();
    motion->yPositions = { 11000, 12000, 13000 };
    PrintJobRunner runner([motion] { return std::make_unique<RecordingMotionController>(motion); });

    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 1;
    config.main.gridColumns = 3;
    const JobResult result = runJobAndWait(runner, config, images, presenter);

    expect(result.success, "single-row legacy scan should complete");
    const QStringList expectedPresentation = {
        "prepare", "vblank", "present:0", "vblank", "present:1", "vblank", "present:2"
    };
    expect(presenter->events.mid(0, expectedPresentation.size()) == expectedPresentation,
        "single-row scan should present every frame with legacy VBlank cadence");
    expect(eventIndex(motion->events, "arm:12000:2147483647") >= 0,
        "forward scan should arm the V2 exposure window");
    expect(eventIndex(motion->events, "move:1:43000") >= 0,
        "forward scan should drive the Y axis through its full row target");
    expect(eventIndex(motion->events, "disarm") > eventIndex(motion->events, "wait:1"),
        "Y movement must stop before the row exposure window is disarmed");
    expect(motion->events.lastIndexOf("o1:0") > eventIndex(motion->events, "disarm"),
        "row cleanup must force O1 low after disarming");
    expect(eventIndex(motion->events, "move:0:0") >= 0 && eventIndex(motion->events, "move:1:0") >= 0,
        "completed job must return X and Y to zero");
    expect(eventIndex(motion->events, "shutdown") >= 0, "completed job must shut down the control card");
}

void testRunnerReversesSecondRow()
{
    ElementalMemoryResult memory = makeMemoryResult(6);
    QString error;
    const PrintImageSet images = makePrintImageSetFromElementalMemory(memory, &error);
    auto presenter = std::make_shared<RecordingFramePresenter>();
    auto motion = std::make_shared<RecordingMotionState>();
    motion->yPositions = { 11000, 12000, 13000, 17000, 16000, 15000 };
    PrintJobRunner runner([motion] { return std::make_unique<RecordingMotionController>(motion); });

    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 2;
    config.main.gridColumns = 3;
    const JobResult result = runJobAndWait(runner, config, images, presenter);

    expect(result.success, "two-row legacy scan should complete");
    const QStringList presented = {
        "present:0", "present:1", "present:2", "present:5", "present:4", "present:3"
    };
    QStringList actual;
    for (const QString& event : presenter->events) {
        if (event.startsWith("present:")) {
            actual << event;
        }
    }
    expect(actual == presented, "second row must use the legacy reverse image order");
    expect(eventIndex(motion->events, "move:0:1000") >= 0,
        "X must advance by one row between serpentine scans");
    expect(eventIndex(motion->events, "move:0:1000") < eventIndex(motion->events, "move:1:0"),
        "the X row step must occur before the reverse Y scan begins");
}

void testRunnerCancelsWithO1LowAndReturnToZero()
{
    ElementalMemoryResult memory = makeMemoryResult(3);
    QString error;
    const PrintImageSet images = makePrintImageSetFromElementalMemory(memory, &error);
    auto presenter = std::make_shared<RecordingFramePresenter>();
    auto motion = std::make_shared<RecordingMotionState>();
    motion->yPositions = { 11000, 12000, 13000 };
    PrintJobRunner runner([motion] { return std::make_unique<RecordingMotionController>(motion); });
    presenter->onPresent = [&runner] { runner.cancel(); };

    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 1;
    config.main.gridColumns = 3;
    const JobResult result = runJobAndWait(runner, config, images, presenter);

    expect(!result.success, "cancellation should fail the running job");
    expect(eventIndex(motion->events, "disarm") >= 0, "cancellation must disarm the exposure window");
    expect(eventCount(motion->events, "o1:0") >= 2, "cancellation must force O1 low for row and final cleanup");
    expect(eventIndex(motion->events, "move:0:0") >= 0 && eventIndex(motion->events, "move:1:0") >= 0,
        "cancellation must return X and Y to zero");
    expect(eventIndex(motion->events, "shutdown") >= 0, "cancellation must shut down the control card");
}

void testRunnerRejectsMissingSecondScreenPresenter()
{
    ElementalMemoryResult memory = makeMemoryResult(1);
    QString error;
    const PrintImageSet images = makePrintImageSetFromElementalMemory(memory, &error);
    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 1;
    config.main.gridColumns = 1;
    PrintJobRunner runner;

    expect(!runner.start(config, images, nullptr, &error),
        "runner must reject a missing second-screen presenter");
    expect(error == "9030 second-screen presenter is unavailable.",
        "missing second-screen presenter should have a clear synchronous error");
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    testElementalMemoryIsPreferredPrintSource();
    testElementalMemoryCopiesBgrPrintFrame();
    testManualFolderFallbackEnumeratesImages();
    testImc60gProductionProfileMatchesV2();
    testPrintConfigDefaultsMatchV2();
    testPrintConfigMigratesEditableValuesWithoutLegacyShutter();
    testMotionControllerRejectsUnavailableCard();
    testSecondScreenSelectionRequiresNonPrimaryMonitor();
    testLegacyTimingMatchesDefault9030Scan();
    testLegacyTimingRejectsVBlankMismatch();
    testSecondScreenFailureDoesNotInitializeCard();
    testVBlankFailureDoesNotInitializeCard();
    testCardInitializationFailureFinishesWithoutMotion();
    testRunnerExecutesOneRowAndCleansUp();
    testRunnerReversesSecondRow();
    testRunnerCancelsWithO1LowAndReturnToZero();
    testRunnerRejectsMissingSecondScreenPresenter();
    std::cout << "printing module tests passed\n";
    return 0;
}

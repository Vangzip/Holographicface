#include "V2D3DFramePresenter.h"

#include <QApplication>
#include <QImage>
#include <QMutex>
#include <QPainter>
#include <QThread>

#include <atomic>
#include <climits>
#include <cstdio>
#include <functional>
#include <memory>
#include <thread>

namespace {

int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

PrintFrame frame(int width, int height, int stride, PrintPixelFormat format, const QByteArray& bytes)
{
    PrintFrame result;
    result.width = width;
    result.height = height;
    result.stride = stride;
    result.format = format;
    result.pixels = bytes;
    return result;
}

DisplayMonitor primaryDisplay()
{
    return {QRect(0, 0, 1920, 1080), true, true, 1, QStringLiteral("PRIMARY")};
}

DisplayMonitor printDisplay(quintptr handle = 2, const QString& name = QStringLiteral("PRINT"))
{
    return {QRect(1920, 0, 1920, 1080), false, true, handle, name};
}

class InlineDispatcher final : public IPresentationDispatcher
{
public:
    bool invokeSynchronously(const std::function<bool()>& command, QString*) override
    {
        ++calls;
        return command();
    }

    int calls = 0;
};

class RecordingVBlankWaiter final : public IVBlankWaiter
{
public:
    bool waitForPhysicalVBlank(VBlankDiagnostics* diagnostics, QString* errorMessage) override
    {
        ++calls;
        if (diagnostics) {
            diagnostics->hresult = hresult;
            diagnostics->outputDeviceName = QStringLiteral("PRINT");
            diagnostics->adapterIndex = 3;
            diagnostics->outputIndex = 4;
        }
        if (!succeeds && errorMessage) {
            *errorMessage = QStringLiteral("IDXGIOutput::WaitForVBlank failed HRESULT=0x%1")
                .arg(static_cast<quint32>(hresult), 8, 16, QLatin1Char('0'));
        }
        return succeeds;
    }

    bool succeeds = true;
    qint32 hresult = 0;
    int calls = 0;
};

class RecordingBackend final : public IV2D3DBackend
{
public:
    bool prepare(const DisplayMonitor& monitor, const QSize&, PresenterDiagnostics* diagnostics,
        QString* errorMessage) override
    {
        ++prepareCalls;
        events.push_back(QStringLiteral("prepare:%1").arg(monitor.deviceName));
        if (!prepareSucceeds) {
            if (errorMessage) *errorMessage = prepareFailure;
            return false;
        }
        if (diagnostics) diagnostics->prepareHresult = 0;
        preparedMonitor = monitor.nativeMonitor;
        return true;
    }

    bool outputMatches(const DisplayMonitor& monitor, QString* errorMessage) const override
    {
        ++matchCalls;
        if (!matchSucceeds || monitor.nativeMonitor != preparedMonitor) {
            if (errorMessage) *errorMessage = QStringLiteral("Selected DXGI output mismatch or disconnected.");
            return false;
        }
        return true;
    }

    bool uploadAndPresent(const QByteArray& bgra, int width, int height, int rowPitch,
        const QSize&, PresenterDiagnostics* diagnostics, QString* errorMessage) override
    {
        const int activeNow = ++active;
        int observed = maxActive.load();
        while (activeNow > observed && !maxActive.compare_exchange_weak(observed, activeNow)) {}
        if (blockFirst && uploadCalls == 0) {
            firstEntered.store(true);
            while (!releaseFirst.load()) QThread::yieldCurrentThread();
        }
        ++uploadCalls;
        uploaded = bgra;
        uploadedWidth = width;
        uploadedHeight = height;
        uploadedPitch = rowPitch;
        events.push_back(QStringLiteral("present:%1").arg(static_cast<unsigned char>(bgra.at(0))));
        --active;
        if (!presentSucceeds) {
            if (diagnostics) diagnostics->presentHresult = presentFailureHr;
            if (errorMessage) *errorMessage = presentFailure;
            return false;
        }
        if (diagnostics) {
            diagnostics->presentHresult = 0;
            ++diagnostics->orderedPresentCount;
        }
        return true;
    }

    void shutdown() override
    {
        ++shutdownCalls;
        events.push_back(QStringLiteral("shutdown"));
        preparedMonitor = 0;
    }

    bool prepareSucceeds = true;
    bool matchSucceeds = true;
    bool presentSucceeds = true;
    QString prepareFailure = QStringLiteral("device creation failed");
    QString presentFailure = QStringLiteral("Present failed");
    qint32 presentFailureHr = static_cast<qint32>(0x887A0005u);
    mutable int matchCalls = 0;
    int prepareCalls = 0;
    int uploadCalls = 0;
    int shutdownCalls = 0;
    quintptr preparedMonitor = 0;
    QByteArray uploaded;
    int uploadedWidth = 0;
    int uploadedHeight = 0;
    int uploadedPitch = 0;
    QStringList events;
    std::atomic_int active{0};
    std::atomic_int maxActive{0};
    bool blockFirst = false;
    std::atomic_bool firstEntered{false};
    std::atomic_bool releaseFirst{false};
};

struct Fixture {
    Fixture(const QVector<DisplayMonitor>& displays = {primaryDisplay(), printDisplay()})
        : backend(std::make_shared<RecordingBackend>())
        , waiter(std::make_shared<RecordingVBlankWaiter>())
        , dispatcher(std::make_shared<InlineDispatcher>())
        , presenter(displays, backend, waiter, dispatcher)
    {}

    std::shared_ptr<RecordingBackend> backend;
    std::shared_ptr<RecordingVBlankWaiter> waiter;
    std::shared_ptr<InlineDispatcher> dispatcher;
    V2D3DFramePresenter presenter;
};

PrintFrame validBgr(unsigned char firstBlue = 1)
{
    QByteArray bytes(8, '\0');
    bytes[0] = static_cast<char>(firstBlue);
    bytes[1] = 2;
    bytes[2] = 3;
    bytes[3] = 4;
    bytes[4] = 5;
    bytes[5] = 6;
    bytes[6] = 99;
    bytes[7] = 98;
    return frame(2, 1, 8, PrintPixelFormat::Bgr24, bytes);
}

void testDisplaySelection()
{
    QVector<DisplayMonitor> displays = {
        printDisplay(10, QStringLiteral("LEFT")), primaryDisplay(),
        printDisplay(11, QStringLiteral("RIGHT")),
        {QRect(0, 0, 0, 0), false, true, 12, QStringLiteral("INVALID")},
        {QRect(0, 0, 640, 480), false, false, 13, QStringLiteral("DETACHED")}
    };
    expect(selectV2SecondScreenIndex(displays) == std::optional<int>(2),
        "last attached non-primary valid monitor must be selected exactly as V2");
    expect(!selectV2SecondScreenIndex({primaryDisplay()}).has_value(),
        "primary-only display list must fail closed");
}

void testPrimaryOnlyFailsBeforeBackendConstructionWork()
{
    Fixture f({primaryDisplay()});
    QString error;
    expect(!f.presenter.prepare(validBgr(), QSize(2, 1), &error), "primary-only prepare must fail");
    expect(f.backend->prepareCalls == 0 && f.backend->uploadCalls == 0,
        "primary-only failure must precede renderer/GPU work");
    expect(error.contains(QStringLiteral("non-primary"), Qt::CaseInsensitive),
        "primary-only failure must be actionable");
}

void testBgrAndBgraUploadBytesAndStride()
{
    Fixture f;
    QString error;
    expect(f.presenter.prepare(validBgr(), QSize(2, 1), &error), "BGR24 prepare must succeed");
    expect(f.backend->events == QStringList({QStringLiteral("prepare:PRINT"), QStringLiteral("present:1")}),
        "prepare must create/bind the selected output before the synchronous first present");
    const QByteArray expectedBgr = QByteArray::fromRawData("\x01\x02\x03\xff\x04\x05\x06\xff", 8);
    expect(f.backend->uploaded == expectedBgr && f.backend->uploadedPitch == 8,
        "BGR24 must upload exact BGRA bytes and packed D3D stride");

    QByteArray bgra(12, '\0');
    const char exact[] = {char(9), char(8), char(7), char(6), char(5), char(4), char(3), char(2)};
    std::memcpy(bgra.data(), exact, sizeof(exact));
    bgra[8] = 99;
    expect(f.presenter.present(frame(2, 1, 12, PrintPixelFormat::Bgra32, bgra), QSize(2, 1), &error),
        "BGRA32 present must succeed");
    expect(f.backend->uploaded == QByteArray(exact, sizeof(exact)) && f.backend->uploadedPitch == 8,
        "BGRA32 must preserve exact channel bytes while removing source padding");
}

void testInvalidFramesFailBeforeUpload()
{
    Fixture f;
    QString error;
    expect(f.presenter.prepare(validBgr(), QSize(2, 1), &error), "fixture prepare must succeed");
    const int uploads = f.backend->uploadCalls;
    const QVector<PrintFrame> invalid = {
        frame(2, 1, 6, PrintPixelFormat::Bgr24, {}),
        frame(0, 1, 6, PrintPixelFormat::Bgr24, QByteArray(6, 0)),
        frame(-1, 1, 6, PrintPixelFormat::Bgr24, QByteArray(6, 0)),
        frame(2, 1, 5, PrintPixelFormat::Bgr24, QByteArray(6, 0)),
        frame(2, 2, 6, PrintPixelFormat::Bgr24, QByteArray(11, 0)),
        frame(2, 1, 8, static_cast<PrintPixelFormat>(99), QByteArray(8, 0)),
        frame(16385, 1, 49155, PrintPixelFormat::Bgr24, QByteArray(1, 0)),
        frame(INT_MAX, INT_MAX, INT_MAX, PrintPixelFormat::Bgra32, QByteArray(1, 0))
    };
    for (const PrintFrame& bad : invalid) {
        error.clear();
        expect(!f.presenter.present(bad, QSize(2, 1), &error), "invalid frame must fail");
        expect(!error.isEmpty(), "invalid frame must provide diagnostics");
    }
    expect(f.backend->uploadCalls == uploads, "every invalid frame must fail before GPU upload");
    expect(!f.presenter.present(validBgr(), QSize(0, 1), &error), "invalid target size must fail before upload");
    expect(f.backend->uploadCalls == uploads, "invalid target size must not upload");
}

void testVBlankFailureIsExactAndHasNoFallback()
{
    Fixture f;
    QString error;
    expect(f.presenter.prepare(validBgr(), QSize(2, 1), &error), "fixture prepare must succeed");
    f.waiter->succeeds = false;
    f.waiter->hresult = static_cast<qint32>(0x887A0001u);
    expect(!f.presenter.waitForDisplayFrame(&error), "DXGI VBlank failure must remain a failure");
    expect(f.presenter.diagnostics().vblank.hresult == f.waiter->hresult,
        "exact DXGI HRESULT must be propagated");
    expect(error.contains(QStringLiteral("887a0001"), Qt::CaseInsensitive),
        "VBlank diagnostic must include exact HRESULT");
    expect(f.waiter->calls == 1, "VBlank failure must not retry through another synchronization method");
}

void testRowAnchorConsumesExactlyOnePhysicalWait()
{
    Fixture f;
    QString error;
    expect(f.presenter.prepare(validBgr(), QSize(2, 1), &error), "fixture prepare must succeed");
    V2RowVBlankAnchor anchor;
    expect(f.presenter.acquireRowAnchor(&anchor, &error), "row anchor must acquire a physical VBlank");
    expect(f.waiter->calls == 1, "row anchor acquisition must perform one physical wait");
    expect(f.presenter.waitForRowSlot(0, &anchor, &error), "first slot must consume row anchor");
    expect(f.waiter->calls == 1, "first row slot must not wait a second time");
    expect(f.presenter.waitForRowSlot(1, &anchor, &error), "later slot must wait physically");
    expect(f.waiter->calls == 2, "later row slot must acquire a new physical VBlank");
    expect(!f.presenter.waitForRowSlot(0, &anchor, &error), "consumed anchor must not be reusable");
}

void testFailuresMismatchAndShutdownAreFailClosed()
{
    const QStringList stages = {QStringLiteral("device"), QStringLiteral("swap-chain"),
        QStringLiteral("shader"), QStringLiteral("render-target")};
    for (const QString& stage : stages) {
        Fixture f;
        f.backend->prepareSucceeds = false;
        f.backend->prepareFailure = stage + QStringLiteral(" creation failed");
        QString error;
        expect(!f.presenter.prepare(validBgr(), QSize(2, 1), &error), "prepare stage failure must fail closed");
        expect(error.contains(stage), "prepare stage diagnostics must propagate");
        expect(!f.presenter.isReady(), "diagnostics must never become readiness");
        f.presenter.shutdown();
        f.presenter.shutdown();
        expect(f.backend->shutdownCalls == 1, "shutdown after partial prepare must be idempotent");
    }

    Fixture mismatch;
    mismatch.backend->matchSucceeds = false;
    QString error;
    expect(!mismatch.presenter.prepare(validBgr(), QSize(2, 1), &error),
        "selected output mismatch/disconnect must fail closed");
    expect(mismatch.backend->uploadCalls == 0, "output mismatch must fail before upload");

    Fixture presentFailure;
    expect(presentFailure.presenter.prepare(validBgr(), QSize(2, 1), &error), "fixture prepare must succeed");
    presentFailure.backend->presentSucceeds = false;
    expect(!presentFailure.presenter.present(validBgr(7), QSize(2, 1), &error), "Present failure must propagate");
    expect(presentFailure.presenter.diagnostics().presentHresult == presentFailure.backend->presentFailureHr,
        "Present HRESULT must remain diagnostic evidence");
    expect(!presentFailure.presenter.isReady(), "Present failure must clear readiness");
}

void testSerializedCommandsCannotOvertake()
{
    Fixture f;
    QString error;
    expect(f.presenter.prepare(validBgr(), QSize(2, 1), &error), "fixture prepare must succeed");
    f.backend->blockFirst = true;
    f.backend->uploadCalls = 0;
    f.backend->events.clear();
    std::thread first([&] { QString local; f.presenter.present(validBgr(21), QSize(2, 1), &local); });
    while (!f.backend->firstEntered.load()) QThread::yieldCurrentThread();
    std::thread second([&] { QString local; f.presenter.present(validBgr(22), QSize(2, 1), &local); });
    QThread::msleep(10);
    f.backend->releaseFirst.store(true);
    first.join();
    second.join();
    expect(f.backend->maxActive.load() == 1, "cross-thread renderer commands must have one serialized owner");
    expect(f.backend->events == QStringList({QStringLiteral("present:21"), QStringLiteral("present:22")}),
        "a later synchronous present must not overtake an earlier present");
}

int runContractTests()
{
    testDisplaySelection();
    testPrimaryOnlyFailsBeforeBackendConstructionWork();
    testBgrAndBgraUploadBytesAndStride();
    testInvalidFramesFailBeforeUpload();
    testVBlankFailureIsExactAndHasNoFallback();
    testRowAnchorConsumesExactlyOnePhysicalWait();
    testFailuresMismatchAndShutdownAreFailClosed();
    testSerializedCommandsCannotOvertake();
    if (failures == 0) std::puts("All V2 presenter contract tests passed.");
    return failures == 0 ? 0 : 1;
}

int runDisplaySmoke()
{
    V2D3DFramePresenter presenter;
    QString error;
    QVector<QImage> images;
    for (int index = 0; index < 10; ++index) {
        QImage image(640, 480, QImage::Format_ARGB32);
        image.fill(QColor::fromHsv(index * 36, 220, 220));
        QPainter painter(&image);
        QFont font = painter.font();
        font.setPixelSize(240);
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(Qt::white);
        painter.drawText(image.rect(), Qt::AlignCenter, QString::number(index));
        images.push_back(image);
    }

    auto toFrame = [](const QImage& image) {
        PrintFrame result;
        result.width = image.width();
        result.height = image.height();
        result.stride = image.bytesPerLine();
        result.format = PrintPixelFormat::Bgra32;
        result.pixels = QByteArray(reinterpret_cast<const char*>(image.constBits()), image.sizeInBytes());
        return result;
    };

    if (!presenter.prepare(toFrame(images.first()), images.first().size(), &error)) {
        std::fprintf(stderr, "display-smoke prepare failed: %s\n", qPrintable(error));
        presenter.shutdown();
        return 2;
    }
    int ordered = 1;
    if (!presenter.waitForDisplayFrame(&error)) {
        std::fprintf(stderr, "display-smoke frame 0 VBlank failed: %s\n", qPrintable(error));
        presenter.shutdown();
        return 3;
    }
    for (int index = 1; index < images.size(); ++index) {
        if (!presenter.present(toFrame(images.at(index)), images.at(index).size(), &error)
            || !presenter.waitForDisplayFrame(&error)) {
            std::fprintf(stderr, "display-smoke frame %d failed: %s\n", index, qPrintable(error));
            presenter.shutdown();
            return 3;
        }
        ++ordered;
    }
    const PresenterDiagnostics d = presenter.diagnostics();
    std::printf("display-smoke monitor=%s adapter=%d output=%d vblank_hr=0x%08x "
                "frame_stats_hr=0x%08x present_count=%u refresh_count=%u latency_wait=%lld ordered=%d\n",
        qPrintable(d.selectedOutputDeviceName), d.vblank.adapterIndex, d.vblank.outputIndex,
        static_cast<unsigned>(d.vblank.hresult), static_cast<unsigned>(d.frameStatisticsHresult),
        d.presentCount, d.presentRefreshCount, static_cast<long long>(d.frameLatencyWaitResult), ordered);
    presenter.shutdown();
    return ordered == 10 ? 0 : 4;
}

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    if (app.arguments().contains(QStringLiteral("--print-display-smoke"))) return runDisplaySmoke();
    return runContractTests();
}

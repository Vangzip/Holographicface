#include "LegacySecondScreenPresenter.h"

#include "LegacyD3DImageRenderer.h"
#include "SecondScreenSelection.h"

#include <QMetaObject>
#include <QThread>
#include <QWidget>

#include <Windows.h>

namespace {

struct NativeDisplayMonitor {
    HMONITOR monitor = nullptr;
    LegacyDisplayMonitor display;
};

BOOL CALLBACK collectDisplayMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM data)
{
    auto* displays = reinterpret_cast<QVector<NativeDisplayMonitor>*>(data);
    MONITORINFO info = {};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfo(monitor, &info)) {
        return TRUE;
    }
    displays->push_back({
        monitor,
        { QRect(
            info.rcMonitor.left,
            info.rcMonitor.top,
            info.rcMonitor.right - info.rcMonitor.left,
            info.rcMonitor.bottom - info.rcMonitor.top),
          (info.dwFlags & MONITORINFOF_PRIMARY) != 0 }
    });
    return TRUE;
}

void setMessage(QString* errorMessage, const QString& message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}

} // namespace

LegacySecondScreenPresenter::LegacySecondScreenPresenter(QObject* parent)
    : QObject(parent)
{
}

LegacySecondScreenPresenter::~LegacySecondScreenPresenter()
{
    shutdown();
}

bool LegacySecondScreenPresenter::prepare(
    const PrintFrame& firstFrame,
    const QSize& targetSize,
    QString* errorMessage)
{
    return invokeOnPresenterThread(
        [this, &firstFrame, &targetSize](QString* localError) {
            return prepareOnPresenterThread(firstFrame, targetSize, localError);
        },
        errorMessage);
}

bool LegacySecondScreenPresenter::present(
    const PrintFrame& frame,
    const QSize& targetSize,
    QString* errorMessage)
{
    return invokeOnPresenterThread(
        [this, &frame, &targetSize](QString* localError) {
            return presentOnPresenterThread(frame, targetSize, localError);
        },
        errorMessage);
}

bool LegacySecondScreenPresenter::waitForVBlank(QString* errorMessage)
{
    return invokeOnPresenterThread(
        [this](QString* localError) {
            return waitForVBlankOnPresenterThread(localError);
        },
        errorMessage);
}

void LegacySecondScreenPresenter::shutdown()
{
    QString ignoredError;
    invokeOnPresenterThread(
        [this](QString*) {
            shutdownOnPresenterThread();
            return true;
        },
        &ignoredError);
}

bool LegacySecondScreenPresenter::invokeOnPresenterThread(
    const std::function<bool(QString*)>& action,
    QString* errorMessage)
{
    QString localError;
    bool result = false;
    const auto invoke = [&] {
        result = action(&localError);
    };
    if (QThread::currentThread() == thread()) {
        invoke();
    } else {
        QMetaObject::invokeMethod(this, invoke, Qt::BlockingQueuedConnection);
    }
    if (!result) {
        setMessage(errorMessage, localError.isEmpty() ? "Second-screen presenter operation failed." : localError);
    } else if (errorMessage) {
        errorMessage->clear();
    }
    return result;
}

bool LegacySecondScreenPresenter::prepareOnPresenterThread(
    const PrintFrame& firstFrame,
    const QSize& targetSize,
    QString* errorMessage)
{
    if (!firstFrame.isValid()) {
        setMessage(errorMessage, "The first print frame is invalid.");
        return false;
    }

    QVector<NativeDisplayMonitor> nativeDisplays;
    EnumDisplayMonitors(nullptr, nullptr, collectDisplayMonitor, reinterpret_cast<LPARAM>(&nativeDisplays));
    QVector<LegacyDisplayMonitor> displays;
    displays.reserve(nativeDisplays.size());
    for (const NativeDisplayMonitor& monitor : nativeDisplays) {
        displays.push_back(monitor.display);
    }
    const std::optional<int> selectedIndex = selectLegacySecondScreenIndex(displays);
    if (!selectedIndex.has_value()) {
        setMessage(errorMessage, "A non-primary display is required for 9030 printing.");
        return false;
    }

    shutdownOnPresenterThread();
    displayGeometry_ = nativeDisplays.at(*selectedIndex).display.geometry;
    displayWindow_ = std::make_unique<QWidget>(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    displayWindow_->setAttribute(Qt::WA_NativeWindow);
    displayWindow_->setStyleSheet("background-color: black;");
    displayWindow_->setGeometry(displayGeometry_);
    displayWindow_->show();

    imageWindow_ = std::make_unique<QWidget>(displayWindow_.get());
    imageWindow_->setAttribute(Qt::WA_NativeWindow);
    imageWindow_->setStyleSheet("background-color: black;");
    if (!resizeTargetWindow(targetSize, errorMessage)) {
        shutdownOnPresenterThread();
        return false;
    }

    renderer_ = std::make_unique<LegacyD3DImageRenderer>();
    const HWND imageHwnd = reinterpret_cast<HWND>(imageWindow_->winId());
    if (!renderer_->initialize(imageHwnd, targetSize.width(), targetSize.height())) {
        setMessage(errorMessage, "D3D11 second-screen renderer initialization failed.");
        shutdownOnPresenterThread();
        return false;
    }
    return presentOnPresenterThread(firstFrame, targetSize, errorMessage);
}

bool LegacySecondScreenPresenter::presentOnPresenterThread(
    const PrintFrame& frame,
    const QSize& targetSize,
    QString* errorMessage)
{
    if (!renderer_ || !frame.isValid()) {
        setMessage(errorMessage, "D3D11 second-screen renderer is not ready.");
        return false;
    }
    if (!resizeTargetWindow(targetSize, errorMessage) || !renderer_->resize(targetSize.width(), targetSize.height())) {
        setMessage(errorMessage, "D3D11 second-screen renderer could not resize.");
        return false;
    }
    if (!renderer_->renderFrame(frame)) {
        setMessage(errorMessage, "D3D11 second-screen renderer could not present a print frame.");
        return false;
    }
    return true;
}

bool LegacySecondScreenPresenter::waitForVBlankOnPresenterThread(QString* errorMessage)
{
    if (!renderer_ || !renderer_->waitForVBlank()) {
        setMessage(errorMessage, "Second-screen VSync is not available.");
        return false;
    }
    return true;
}

void LegacySecondScreenPresenter::shutdownOnPresenterThread()
{
    renderer_.reset();
    if (imageWindow_) {
        imageWindow_->hide();
        imageWindow_.reset();
    }
    if (displayWindow_) {
        displayWindow_->hide();
        displayWindow_.reset();
    }
    displayGeometry_ = {};
    targetSize_ = {};
}

bool LegacySecondScreenPresenter::resizeTargetWindow(const QSize& targetSize, QString* errorMessage)
{
    if (!displayWindow_ || !imageWindow_ || targetSize.width() <= 0 || targetSize.height() <= 0) {
        setMessage(errorMessage, "Second-screen image size is invalid.");
        return false;
    }
    imageWindow_->setGeometry(
        (displayGeometry_.width() - targetSize.width()) / 2,
        (displayGeometry_.height() - targetSize.height()) / 2,
        targetSize.width(),
        targetSize.height());
    imageWindow_->show();
    targetSize_ = targetSize;
    return true;
}

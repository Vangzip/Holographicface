#include "CaptureSession.h"

#include "FrameChangeDetector.hpp"
#include "LightFieldCapture.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QStorageInfo>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <opencv2/opencv.hpp>

namespace {

LightFieldCapture* g_captureForShutdown = nullptr;
std::atomic<bool> g_shutdownCleanupDone{false};
std::atomic<bool> g_stopRequested{false};

QString normalizedDir(QString path)
{
    QDir dir(path);
    QString absolute = dir.absolutePath();
    if (!absolute.endsWith('/')) {
        absolute += '/';
    }
    return absolute;
}

qint64 availableDiskSpaceGb(const QString& path)
{
    QStorageInfo storage(path);
    storage.refresh();
    if (!storage.isValid()) {
        return -1;
    }
    return storage.bytesAvailable() / (1024LL * 1024LL * 1024LL);
}

void releaseCaptureOnce() noexcept
{
    bool expected = false;
    if (!g_shutdownCleanupDone.compare_exchange_strong(expected, true)) {
        return;
    }
    LightFieldCapture* capture = g_captureForShutdown;
    g_captureForShutdown = nullptr;
    if (!capture) {
        return;
    }
    try {
        qDebug() << "Cleaning camera resources...";
        capture->release();
    } catch (...) {
    }
}

extern "C" void captureAtExitHandler()
{
    releaseCaptureOnce();
}

#ifdef Q_OS_WIN
static BOOL WINAPI consoleCtrlHandler(DWORD ctrlType)
{
    switch (ctrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        g_stopRequested.store(true, std::memory_order_release);
        if (QCoreApplication* app = QCoreApplication::instance()) {
            QMetaObject::invokeMethod(app, "quit", Qt::QueuedConnection);
        }
        return TRUE;
    default:
        return FALSE;
    }
}
#endif

} // namespace

int runCaptureSession(const CaptureSessionOptions& options)
{
    g_stopRequested.store(false, std::memory_order_release);
    g_shutdownCleanupDone.store(false, std::memory_order_release);

    const QString saveRoot = normalizedDir(options.saveRoot);
    if (!QDir().mkpath(saveRoot)) {
        qDebug() << "Cannot create save directory:" << saveRoot;
        return 1;
    }

    qDebug() << "=== MergeHolo camera capture ===";
    qDebug() << "Save root:" << saveRoot;
    qDebug() << "Camera config:" << options.cameraConfigPath;
    qDebug() << "Min free space:" << options.minFreeSpaceGb << "GB";
    qDebug() << "Save interval:" << options.saveIntervalMs << "ms";
    qDebug() << "Max frames:" << options.maxFrames;
    qDebug() << "Duration seconds:" << options.durationSeconds;

    const qint64 initialSpace = availableDiskSpaceGb(saveRoot);
    if (initialSpace >= 0) {
        qDebug() << "Available disk space:" << initialSpace << "GB";
    }

    LightFieldCapture capture;
    LightFieldCapture::HoloInData config;
    config.iHoloExposeMode = 1;
    config.iHoloExposeVal = 15000;
    config.iHoloId = 0;
    config.dHoloFrameRate = 6.0;
    config.iHoloMissThreshold = 100;
    config.bIsReadTeamptureBySerial = false;
    config.strSerialPort = "";
    config.iSerialBaudRate = 9600;
    config.iSerialDataBits = 8;
    config.iSerialStopBits = 1;
    config.iSerialParity = 0;
    config.strParseCfgPath = QDir::toNativeSeparators(options.cameraConfigPath).toStdString();
    std::replace(config.strParseCfgPath.begin(), config.strParseCfgPath.end(), '\\', '/');

    if (!capture.initialize(&config)) {
        qDebug() << "Camera initialization failed.";
        return 1;
    }

    g_captureForShutdown = &capture;
    std::atexit(captureAtExitHandler);
#ifdef Q_OS_WIN
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#endif

    const QString saveDir2d = saveRoot + "2d/";
    const QString saveDir3d = saveRoot + "3d/";
    const QString saveDirRaw = saveRoot + "raw/";
    if (!QDir().mkpath(saveDir2d) || !QDir().mkpath(saveDir3d) || !QDir().mkpath(saveDirRaw)) {
        qDebug() << "Cannot create capture subdirectories under:" << saveRoot;
        g_captureForShutdown = nullptr;
        return 1;
    }

    cv::Mat previousImage2d;
    QElapsedTimer saveTimer;
    QElapsedTimer totalTimer;
    saveTimer.start();
    totalTimer.start();

    int savedFrames = 0;
    bool isFirstFrame = true;

    while (!g_stopRequested.load(std::memory_order_acquire)) {
        QCoreApplication::processEvents();
        if (options.durationSeconds > 0 && totalTimer.elapsed() >= options.durationSeconds * 1000LL) {
            qDebug() << "Capture duration reached.";
            break;
        }
        if (options.maxFrames > 0 && savedFrames >= options.maxFrames) {
            qDebug() << "Max saved frames reached.";
            break;
        }

        LightFieldCapture::HoloOutData data;
        if (!capture.GetHoloOutData(data)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        cv::Mat image2d = data.img2d.clone();
        cv::Mat image3d = data.img3d.clone();
        if (image2d.empty() || image3d.empty()) {
            continue;
        }

        if (options.showPreview) {
            cv::imshow("mergeholo 2d", image2d);
            cv::imshow("mergeholo 3d", image3d);
            cv::waitKey(1);
        }

        if (!isFirstFrame && saveTimer.elapsed() < options.saveIntervalMs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        bool shouldSave = isFirstFrame;
        double movementArea = 0.0;
        if (!shouldSave && !previousImage2d.empty()) {
            shouldSave = hasSignificantChange(previousImage2d, image2d, false, &movementArea);
        }
        if (!shouldSave) {
            qDebug() << "No significant frame change, movement area:" << movementArea;
            saveTimer.restart();
            isFirstFrame = false;
            continue;
        }

        const qint64 freeGb = availableDiskSpaceGb(saveRoot);
        if (freeGb >= 0 && freeGb < options.minFreeSpaceGb) {
            qDebug() << "Disk free space below threshold:" << freeGb << "GB";
            break;
        }

        const QString stem = QDateTime::currentDateTime().toString("MMdd_HHmmsszzz");
        const QString path2d = saveDir2d + stem + ".jpg";
        const QString path3d = saveDir3d + stem + "_3D.tiff";

        if (image3d.type() != CV_32FC3) {
            cv::Mat converted;
            image3d.convertTo(converted, CV_32FC3);
            image3d = converted;
        }

        std::vector<int> tiffParams;
        tiffParams.push_back(cv::IMWRITE_TIFF_COMPRESSION);
        tiffParams.push_back(1);
        const bool saved2d = cv::imwrite(path2d.toStdString(), image2d);
        const bool saved3d = cv::imwrite(path3d.toStdString(), image3d, tiffParams);
        if (!saved2d || !saved3d) {
            qDebug() << "Failed to save frame:" << stem << "2d=" << saved2d << "3d=" << saved3d;
            g_captureForShutdown = nullptr;
            return 1;
        }

        ++savedFrames;
        previousImage2d = image2d.clone();
        saveTimer.restart();
        isFirstFrame = false;
        qDebug() << "Saved frame" << savedFrames << ":" << stem;
    }

    g_captureForShutdown = nullptr;
    qDebug() << "Capture finished. Saved frames:" << savedFrames;
    return 0;
}

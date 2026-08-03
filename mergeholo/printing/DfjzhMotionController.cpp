#include "DfjzhMotionController.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QThread>

#include <limits>
#include <type_traits>
#include <utility>

namespace {

constexpr unsigned short kBoardNo = 0;
constexpr int kAxisCount = 4;

void setMessage(QString* errorMessage, const QString& message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}

QStringList candidateLibraryPaths(const QString& explicitPath)
{
    if (!explicitPath.isEmpty()) {
        return { explicitPath };
    }

    QStringList paths;
    const QString appDir = QCoreApplication::applicationDirPath();
    paths << QDir(appDir).filePath("DfjzhControlerDll.dll");
    paths << QDir(appDir).filePath("dfjzh6052dll.dll");
    paths << QDir(QDir::currentPath()).filePath("DfjzhControlerDll.dll");
    paths << QDir(QDir::currentPath()).filePath("../9030/DfjzhControlerDll.dll");
    return paths;
}

} // namespace

DfjzhMotionController::DfjzhMotionController(QString libraryPath)
    : libraryPath_(std::move(libraryPath))
{
}

DfjzhMotionController::~DfjzhMotionController()
{
    shutdown();
}

bool DfjzhMotionController::initialize(QString* errorMessage)
{
    if (initialized_) {
        if (errorMessage) {
            errorMessage->clear();
        }
        return true;
    }

    shutdown();
    if (!loadLibrary(errorMessage) || !resolveRequiredFunctions(errorMessage)) {
        shutdown();
        return false;
    }

    const short result = initCard_(kBoardNo, 1, 1, 1, 1, 0);
    if (result != 0) {
        setMessage(errorMessage, "Motion card initialization failed, code: " + QString::number(result));
        shutdown();
        return false;
    }

    initialized_ = true;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

void DfjzhMotionController::shutdown()
{
    if (initialized_) {
        disarmExposureWindow();
        if (exitCard_) {
            exitCard_(kBoardNo);
        }
    }

    initialized_ = false;
    exposureWindowArmed_ = false;
    resetFunctions();
    if (library_.isLoaded()) {
        library_.unload();
    }
}

bool DfjzhMotionController::setCurrentPositionAsOrigin(int axis, QString* errorMessage)
{
    if (!ensureInitialized(errorMessage) || !validAxis(axis, errorMessage)) {
        return false;
    }
    if (home_(kBoardNo, static_cast<unsigned short>(axis)) != 0) {
        setMessage(errorMessage, "Motion card failed to set axis " + QString::number(axis) + " as origin.");
        return false;
    }
    return true;
}

bool DfjzhMotionController::moveTo(
    int axis,
    long targetPulse,
    const PrintAxisConfig& config,
    QString* errorMessage)
{
    if (!ensureInitialized(errorMessage) || !validAxis(axis, errorMessage)) {
        return false;
    }

    const unsigned short axisNo = static_cast<unsigned short>(axis);
    if (setAxisAcc_(kBoardNo, axisNo, config.acceleratedVelocity) != 0
        || setAxisDec_(kBoardNo, axisNo, config.acceleratedVelocity) != 0
        || setAxisVel_(kBoardNo, axisNo, config.speedOfMovement) != 0
        || setAxisStartVel_(kBoardNo, axisNo, config.startSpeed) != 0
        || setAxisStopVel_(kBoardNo, axisNo, config.stopSpeed) != 0
        || setAxisPos_(kBoardNo, axisNo, targetPulse) != 0
        || startAxis_(kBoardNo, axisNo) != 0) {
        setMessage(errorMessage, "Motion card failed to start axis " + QString::number(axis));
        return false;
    }
    return true;
}

bool DfjzhMotionController::stopAxis(int axis, QString* errorMessage)
{
    if (!ensureInitialized(errorMessage) || !validAxis(axis, errorMessage)) {
        return false;
    }
    const unsigned short axisNo = static_cast<unsigned short>(axis);
    if (setAxisStopDec_(kBoardNo, axisNo, 100000) != 0 || ceaseAxis_(kBoardNo, axisNo) != 0) {
        setMessage(errorMessage, "Motion card failed to stop axis " + QString::number(axis));
        return false;
    }
    return true;
}

bool DfjzhMotionController::waitUntilStopped(
    int axis,
    int timeoutMs,
    const std::atomic_bool& cancelRequested,
    QString* errorMessage)
{
    if (!ensureInitialized(errorMessage) || !validAxis(axis, errorMessage)) {
        return false;
    }

    QElapsedTimer timer;
    timer.start();
    while (readAxisState_(kBoardNo, static_cast<unsigned short>(axis)) != 0) {
        if (cancelRequested.load()) {
            stopAxis(axis, nullptr);
            return false;
        }
        if (timeoutMs > 0 && timer.elapsed() > timeoutMs) {
            setMessage(errorMessage, "Timed out waiting for axis " + QString::number(axis));
            return false;
        }
        QThread::msleep(5);
    }
    return true;
}

long DfjzhMotionController::readPosition(int axis) const
{
    if (!initialized_ || axis < 0 || axis >= kAxisCount || !readAxisPos_) {
        return 0;
    }
    return readAxisPos_(kBoardNo, static_cast<unsigned short>(axis));
}

bool DfjzhMotionController::setExposureOutput(bool enabled, int outputIndex, QString* errorMessage)
{
    if (!ensureInitialized(errorMessage)) {
        return false;
    }
    if (outputIndex < 0 || outputIndex > std::numeric_limits<short>::max()) {
        setMessage(errorMessage, "Invalid motion card output index: " + QString::number(outputIndex));
        return false;
    }
    if (writeIoBit_(kBoardNo, enabled ? 1 : 0, static_cast<short>(outputIndex)) != 0) {
        setMessage(errorMessage, "Motion card failed to set exposure output.");
        return false;
    }
    return true;
}

bool DfjzhMotionController::armExposureWindow(long beginPos, long endPos, QString* errorMessage)
{
    if (!ensureInitialized(errorMessage)) {
        return false;
    }

    disarmExposureWindow();
    constexpr unsigned short axisNo = 1;
    constexpr short positionCompareFlag = 6;
    constexpr short compareTheoryPos = 1;
    constexpr short activeHigh = 1;
    constexpr short exposureOutputIndex = 1;

    if (setAxisIo_(kBoardNo, axisNo, positionCompareFlag, compareTheoryPos, activeHigh, exposureOutputIndex) != 0
        || setIoPos_(kBoardNo, axisNo, beginPos, endPos) != 0
        || enableIoPos_(kBoardNo, axisNo, 1) != 0) {
        disarmExposureWindow();
        setMessage(errorMessage, "Motion card failed to arm exposure window.");
        return false;
    }
    exposureWindowArmed_ = true;
    return true;
}

void DfjzhMotionController::disarmExposureWindow()
{
    if (!initialized_) {
        exposureWindowArmed_ = false;
        return;
    }
    enableIoPos_(kBoardNo, 1, 0);
    setAxisIo_(kBoardNo, 1, 6, 0, 1, 1);
    writeIoBit_(kBoardNo, 0, 1);
    exposureWindowArmed_ = false;
}

bool DfjzhMotionController::loadLibrary(QString* errorMessage)
{
    QString lastLoadError;
    for (const QString& candidate : candidateLibraryPaths(libraryPath_)) {
        if (!QFileInfo::exists(candidate)) {
            continue;
        }
        library_.setFileName(candidate);
        if (library_.load()) {
            return true;
        }
        lastLoadError = library_.errorString();
    }

    const QString suffix = lastLoadError.isEmpty() ? QString() : " " + lastLoadError;
    setMessage(errorMessage, "Motion card DLL was not found or could not be loaded." + suffix);
    return false;
}

bool DfjzhMotionController::resolveRequiredFunctions(QString* errorMessage)
{
    auto resolve = [this](auto& target, const char* name) {
        target = reinterpret_cast<std::remove_reference_t<decltype(target)>>(library_.resolve(name));
        return target != nullptr;
    };

    const bool ok = resolve(initCard_, "InitCard_ID")
        && resolve(exitCard_, "ExitCard_ID")
        && resolve(home_, "Home_ID")
        && resolve(setAxisAcc_, "SetAxisAcc_ID")
        && resolve(setAxisDec_, "SetAxisDec_ID")
        && resolve(setAxisVel_, "SetAxisVel_ID")
        && resolve(setAxisStartVel_, "SetAxisStartVel_ID")
        && resolve(setAxisStopVel_, "SetAxisStopVel_ID")
        && resolve(setAxisStopDec_, "SetAxisStopDec_ID")
        && resolve(setAxisPos_, "SetAxisPos_ID")
        && resolve(startAxis_, "StartAxis_ID")
        && resolve(ceaseAxis_, "CeaseAxis_ID")
        && resolve(readAxisState_, "ReadAxisState_ID")
        && resolve(readAxisPos_, "ReadAxisPos_ID")
        && resolve(writeIoBit_, "WriteIoBit_ID")
        && resolve(setAxisIo_, "SetAxisIO_ID")
        && resolve(setIoPos_, "Set_IO_Pos_ID")
        && resolve(enableIoPos_, "Enable_IO_Pos_ID");

    if (!ok) {
        setMessage(errorMessage, "Motion card DLL is missing one or more required functions.");
        return false;
    }
    return true;
}

bool DfjzhMotionController::ensureInitialized(QString* errorMessage) const
{
    if (!initialized_) {
        setMessage(errorMessage, "Motion card is not initialized.");
        return false;
    }
    return true;
}

bool DfjzhMotionController::validAxis(int axis, QString* errorMessage) const
{
    if (axis < 0 || axis >= kAxisCount) {
        setMessage(errorMessage, "Invalid motion axis: " + QString::number(axis));
        return false;
    }
    return true;
}

void DfjzhMotionController::resetFunctions()
{
    initCard_ = nullptr;
    exitCard_ = nullptr;
    home_ = nullptr;
    setAxisAcc_ = nullptr;
    setAxisDec_ = nullptr;
    setAxisVel_ = nullptr;
    setAxisStartVel_ = nullptr;
    setAxisStopVel_ = nullptr;
    setAxisStopDec_ = nullptr;
    setAxisPos_ = nullptr;
    startAxis_ = nullptr;
    ceaseAxis_ = nullptr;
    readAxisState_ = nullptr;
    readAxisPos_ = nullptr;
    writeIoBit_ = nullptr;
    setAxisIo_ = nullptr;
    setIoPos_ = nullptr;
    enableIoPos_ = nullptr;
}

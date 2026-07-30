#include "Imc60gMotionController.h"

#include "IImc60gApi.h"

#include <QMutexLocker>
#include <QThread>

#include <chrono>
#include <cmath>
#include <limits>

namespace {

constexpr unsigned int kAxisAlarm = 0x00000001;
constexpr unsigned int kAxisBusy = 0x00000004;
constexpr unsigned int kPositiveLimit = 0x00000010;
constexpr unsigned int kNegativeLimit = 0x00000020;
constexpr unsigned int kPositiveLimitStopReason = 0x04;
constexpr unsigned int kNegativeLimitStopReason = 0x05;
constexpr unsigned int kDiStopReason = 0x0b;
constexpr short kNoAxis = -1;

QMutex gSdkOwnerMutex;
Imc60gMotionController* gSdkOwner = nullptr;
bool gSdkShutdownPoisoned = false;
QString gSdkShutdownPoisonReason;

class SystemImc60gClock final : public IImc60gClock {
public:
    qint64 nowMs() const override
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - origin_).count();
    }

    void sleepMs(unsigned long milliseconds) override
    {
        QThread::msleep(milliseconds);
    }

private:
    const std::chrono::steady_clock::time_point origin_ =
        std::chrono::steady_clock::now();
};

void setError(QString* destination, const QString& message)
{
    if (destination) {
        *destination = message;
    }
}

QString errorCodeText(int code)
{
    switch (code) {
    case 0x0001: return "ERR_TRANSMIT: command transmission error";
    case 0x0002: return "ERR_UNKNOWN: unsupported command";
    case 0x0003: return "ERR_PARSE: command parse error";
    case 0x0006: return "ERR_CMD_EXECUTE: command execution error";
    case 0x0009: return "ERR_INPUT: invalid pointer parameter";
    case 0x0020: return "ERR_INDEX_OUTRANG: index out of range";
    case 0x0021: return "ERR_CARD_INDEX_OUTRANG: card index out of range";
    case 0x0022: return "ERR_SLAVE_INDEX_OUTRANG: slave index out of range";
    case 0x0023: return "ERR_AX_INDEX_OUTRANG: axis index out of range";
    case 0x0040: return "ERR_PARAM_OUTRANG: parameter out of range";
    case 0x0041: return "ERR_COUNT_OUTRANG: count out of range";
    case 0x0046: return "ERR_MODE_OUTRANG: mode out of range";
    case 0x0065: return "ERR_INVERSE_PARA: inverse parameter must be 0 or 1";
    case 0x0100: return "ERR_VEL_OUTRANG: velocity out of range";
    case 0x0101: return "ERR_ACC_OUTRANG: acceleration out of range";
    case 0x0102: return "ERR_DEC_OUTRANG: deceleration out of range";
    case 0x0103: return "ERR_TGTPOS_OUTRANG: target position out of range";
    case 0x0161: return "ERR_ECAT_AX_CNT_OUTRANG: EtherCAT axis count out of range";
    case 0x0200: return "ERR_ECAT_MASTER_LINK: EtherCAT master link error";
    case 0x0201: return "ERR_ECAT_SLAVE_LINK: EtherCAT slave link error";
    default: return "unknown code from IMC errorcode.h";
    }
}

QString formatApiFailure(
    const char* functionName, int card, short axis, int code)
{
    const QString axisText = axis >= 0 ? QString::number(axis) : "n/a";
    return QString("%1 failed (card=%2, physical-axis=%3, code=%4 [0x%5], %6)")
        .arg(functionName)
        .arg(card)
        .arg(axisText)
        .arg(code)
        .arg(static_cast<unsigned int>(code), 0, 16)
        .arg(errorCodeText(code));
}

bool reachedLimit(unsigned int status, unsigned int reason, int direction)
{
    if (direction < 0) {
        return (status & kNegativeLimit) != 0 || reason == kNegativeLimitStopReason;
    }
    return (status & kPositiveLimit) != 0 || reason == kPositiveLimitStopReason;
}

qlonglong absolutePulseDifference(int lhs, int rhs)
{
    return std::abs(static_cast<qlonglong>(lhs) - static_cast<qlonglong>(rhs));
}

bool sdkIsPoisoned(QString* reason)
{
    QMutexLocker ownerLock(&gSdkOwnerMutex);
    if (reason) {
        *reason = gSdkShutdownPoisonReason;
    }
    return gSdkShutdownPoisoned;
}

} // namespace

Imc60gMotionController::Imc60gMotionController(
    IImc60gApi* api, const PrintHardwareProfile& profile, IImc60gClock* clock)
    : api_(api)
    , profile_(profile)
    , clock_(clock)
{
    if (!clock_) {
        ownedClock_ = std::make_unique<SystemImc60gClock>();
        clock_ = ownedClock_.get();
    }
}

Imc60gMotionController::~Imc60gMotionController()
{
    disconnect(nullptr);
}

Imc60gConnectionState Imc60gMotionController::state() const
{
    QMutexLocker locker(&mutex_);
    return state_;
}

bool Imc60gMotionController::acquireOwnership(QString* errorMessage)
{
    QMutexLocker ownerLock(&gSdkOwnerMutex);
    if (gSdkShutdownPoisoned) {
        setError(errorMessage,
            "IMC60G SDK access is blocked after an unverified shutdown; restart the process. "
            + gSdkShutdownPoisonReason);
        return false;
    }
    if (gSdkOwner && gSdkOwner != this) {
        setError(errorMessage,
            "IMC60G SDK card 0 is already owned by another motion controller.");
        return false;
    }
    gSdkOwner = this;
    return true;
}

void Imc60gMotionController::releaseOwnership()
{
    QMutexLocker ownerLock(&gSdkOwnerMutex);
    if (gSdkOwner == this) {
        gSdkOwner = nullptr;
    }
}

void Imc60gMotionController::poisonOwnership(const QString& reason)
{
    QMutexLocker ownerLock(&gSdkOwnerMutex);
    gSdkShutdownPoisoned = true;
    gSdkShutdownPoisonReason = reason;
    if (gSdkOwner == this) {
        gSdkOwner = nullptr;
    }
}

bool Imc60gMotionController::callSucceeded(
    int code, const char* functionName, short axis, QString* errorMessage) const
{
    if (code == 0) {
        return true;
    }
    setError(errorMessage,
        formatApiFailure(functionName, profile_.cardIndex, axis, code));
    return false;
}

bool Imc60gMotionController::connectAndHome(QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    if (errorMessage) {
        errorMessage->clear();
    }
    cancelRequested_.store(false);

    QString validationError;
    if (!validatePrintHardwareProfile(profile_, &validationError)) {
        state_ = Imc60gConnectionState::Fault;
        setError(errorMessage, validationError);
        return false;
    }
    if (state_ == Imc60gConnectionState::Ready) {
        return true;
    }
    if (!api_) {
        state_ = Imc60gConnectionState::Fault;
        setError(errorMessage, "IMC60G API is unavailable; no hardware call was made.");
        return false;
    }
    if (!acquireOwnership(errorMessage)) {
        state_ = Imc60gConnectionState::Fault;
        return false;
    }

    state_ = Imc60gConnectionState::Connecting;
    unsigned int cards = 0;
    if (!callSucceeded(api_->getCardsNum(&cards), "IMC_GetCardsNum", kNoAxis, errorMessage)) {
        goto fail;
    }
    if (cards == 0) {
        setError(errorMessage, "IMC_GetCardsNum found no IMC60G cards (card=0, count=0).");
        goto fail;
    }
    if (!callSucceeded(api_->openCard(0), "IMC_OpenCard", kNoAxis, errorMessage)) {
        goto fail;
    }
    cardOpened_ = true;
    ethercatTouched_ = true;
    if (!callSucceeded(api_->scanEthercat(0, 40), "IMC_ScanCardEcat", kNoAxis, errorMessage)
        || !callSucceeded(api_->initEthercat(0), "IMC_InitEcatComm", kNoAxis, errorMessage)
        || !callSucceeded(api_->startEthercat(0), "IMC_StartEcatComm", kNoAxis, errorMessage)
        || !callSucceeded(api_->setEmergencyLevel(0, 1), "IMC_SetEmgTrigLevelInv", kNoAxis, errorMessage)
        || !callSucceeded(api_->clearAxisStatus(0, 0), "IMC_ClrAxSts", 0, errorMessage)
        || !callSucceeded(api_->clearAxisStatus(0, 1), "IMC_ClrAxSts", 1, errorMessage)) {
        goto fail;
    }
    // Once Servo On is issued, shutdown must treat the axis as potentially
    // enabled even when the return code is nonzero.
    servoEnabled_[0] = true;
    if (!callSucceeded(api_->servoOn(0, 0), "IMC_ServoOn", 0, errorMessage)) {
        goto fail;
    }
    servoEnabled_[1] = true;
    if (!callSucceeded(api_->servoOn(0, 1), "IMC_ServoOn", 1, errorMessage)) {
        goto fail;
    }

    state_ = Imc60gConnectionState::Homing;
    if (!homeAxis(PrintHardwareProfile::LogicalAxis::Y, errorMessage)
        || !homeAxis(PrintHardwareProfile::LogicalAxis::X, errorMessage)) {
        goto fail;
    }
    state_ = Imc60gConnectionState::Ready;
    return true;

fail:
    {
        const QString primaryError = errorMessage ? *errorMessage : QString();
        QString cleanupError;
        const bool cleanupVerified = cleanupHardware(&cleanupError);
        if (!cleanupVerified) {
            poisonOwnership(cleanupError);
            setError(errorMessage,
                primaryError + "; shutdown unverified: " + cleanupError);
        } else {
            releaseOwnership();
        }
        state_ = Imc60gConnectionState::Fault;
        return false;
    }
}

bool Imc60gMotionController::cancellationRequested(
    short activeAxis, QString* errorMessage)
{
    if (!cancelRequested_.load()) {
        return false;
    }
    const int stopCode = api_->stop(0, activeAxis, 1);
    if (stopCode != 0) {
        setError(errorMessage,
            "IMC60G homing cancelled; "
            + formatApiFailure("IMC_StopMove", profile_.cardIndex, activeAxis, stopCode));
    } else {
        setError(errorMessage,
            QString("IMC60G homing cancelled (card=%1, physical-axis=%2).")
                .arg(profile_.cardIndex).arg(activeAxis));
    }
    return true;
}

bool Imc60gMotionController::homeAxis(
    PrintHardwareProfile::LogicalAxis logicalAxis, QString* errorMessage)
{
    const short axis = physicalAxis(logicalAxis);
    if (axis < 0) {
        setError(errorMessage, "IMC60G homing rejected: configured physical axis is missing.");
        return false;
    }
    if (cancellationRequested(axis, errorMessage)) {
        return false;
    }
    const int direction = homeDirection(logicalAxis);
    if (direction != -1 && direction != 1) {
        setError(errorMessage,
            QString("IMC60G homing rejected invalid direction (card=0, physical-axis=%1).")
                .arg(axis));
        return false;
    }

    if (!callSucceeded(api_->stop(0, axis, 0), "IMC_StopMove", axis, errorMessage)
        || !callSucceeded(api_->clearAxisStatus(0, axis), "IMC_ClrAxSts", axis, errorMessage)) {
        return false;
    }

    unsigned int status = 0;
    unsigned int reason = 0;
    int startPlanned = 0;
    int startEncoder = 0;
    if (!callSucceeded(api_->axisStatus(0, axis, &status), "IMC_GetAxSts", axis, errorMessage)
        || !callSucceeded(api_->stopReason(0, axis, &reason), "IMC_GetAxStopReason", axis, errorMessage)
        || !callSucceeded(api_->plannedPosition(0, axis, &startPlanned), "IMC_GetAxPrfPos32", axis, errorMessage)
        || !callSucceeded(api_->encoderPosition(0, axis, &startEncoder), "IMC_GetAxEncPos32", axis, errorMessage)) {
        return false;
    }

    bool limitReached = reachedLimit(status, reason, direction);
    if (!limitReached) {
        if (!callSucceeded(api_->setMotionProfile(0, axis, profile_.homeSpeed,
                profile_.homeAcceleration, profile_.homeDeceleration, 0),
                "IMC_SetAxMvPara", axis, errorMessage)
            || !callSucceeded(api_->configureJog(0, axis),
                "IMC_JogPrf", axis, errorMessage)
            || !callSucceeded(api_->startJogMove(0, axis, direction),
                "IMC_StartJogMove", axis, errorMessage)) {
            return false;
        }

        const qint64 timeoutStart = clock_->nowMs();
        qint64 encoderStableSince = timeoutStart;
        int lastEncoder = startEncoder;
        while (clock_->nowMs() - timeoutStart <= profile_.homeTimeoutMs) {
            if (cancellationRequested(axis, errorMessage)) {
                return false;
            }
            int planned = 0;
            int encoder = 0;
            if (!callSucceeded(api_->axisStatus(0, axis, &status), "IMC_GetAxSts", axis, errorMessage)
                || !callSucceeded(api_->stopReason(0, axis, &reason), "IMC_GetAxStopReason", axis, errorMessage)
                || !callSucceeded(api_->plannedPosition(0, axis, &planned), "IMC_GetAxPrfPos32", axis, errorMessage)
                || !callSucceeded(api_->encoderPosition(0, axis, &encoder), "IMC_GetAxEncPos32", axis, errorMessage)) {
                return false;
            }
            if (absolutePulseDifference(encoder, lastEncoder) > 3) {
                lastEncoder = encoder;
                encoderStableSince = clock_->nowMs();
            }
            if (reachedLimit(status, reason, direction) || reason == kDiStopReason) {
                limitReached = true;
                break;
            }
            const bool movedEnough =
                absolutePulseDifference(encoder, startEncoder) >= profile_.homeMinimumMove;
            const bool stoppedAndStable = (status & kAxisBusy) == 0
                && clock_->nowMs() - encoderStableSince >= profile_.homeStableMs;
            if (movedEnough && stoppedAndStable) {
                limitReached = true;
                break;
            }
            if ((status & kAxisAlarm) != 0) {
                setError(errorMessage,
                    QString("IMC60G homing alarm (card=0, physical-axis=%1, status=0x%2, stop-reason=%3).")
                        .arg(axis).arg(status, 0, 16).arg(reason));
                return false;
            }
            clock_->sleepMs(1);
        }
        if (!limitReached) {
            setError(errorMessage,
                QString("IMC60G homing timed out (card=0, physical-axis=%1, timeout-ms=%2, status=0x%3, stop-reason=%4).")
                    .arg(axis).arg(profile_.homeTimeoutMs).arg(status, 0, 16).arg(reason));
            return false;
        }
    }

    if (cancellationRequested(axis, errorMessage)) {
        return false;
    }
    if (!callSucceeded(api_->stop(0, axis, 0), "IMC_StopMove", axis, errorMessage)) {
        return false;
    }

    int currentPosition = 0;
    if (!callSucceeded(api_->plannedPosition(0, axis, &currentPosition),
            "IMC_GetAxPrfPos32", axis, errorMessage)
        || !callSucceeded(api_->clearAxisStatus(0, axis),
            "IMC_ClrAxSts", axis, errorMessage)
        || !callSucceeded(api_->setMotionProfile(0, axis, profile_.homeBackoffSpeed,
            profile_.homeAcceleration, profile_.homeDeceleration, 0),
            "IMC_SetAxMvPara", axis, errorMessage)) {
        return false;
    }
    const int backoffTarget = currentPosition + (-direction) * homeBackoff(logicalAxis);
    if (!callSucceeded(api_->startPtp(0, axis, backoffTarget),
            "IMC_StartPtpMove", axis, errorMessage)) {
        return false;
    }

    const qint64 backoffStart = clock_->nowMs();
    bool backoffComplete = false;
    while (clock_->nowMs() - backoffStart <= profile_.homeBackoffTimeoutMs) {
        if (cancellationRequested(axis, errorMessage)) {
            return false;
        }
        if (!callSucceeded(api_->axisStatus(0, axis, &status),
                "IMC_GetAxSts", axis, errorMessage)) {
            return false;
        }
        if ((status & kAxisBusy) == 0) {
            backoffComplete = true;
            break;
        }
        clock_->sleepMs(1);
    }
    if (!backoffComplete) {
        setError(errorMessage,
            QString("IMC60G backoff timed out (card=0, physical-axis=%1, timeout-ms=%2).")
                .arg(axis).arg(profile_.homeBackoffTimeoutMs));
        return false;
    }
    if (cancellationRequested(axis, errorMessage)) {
        return false;
    }
    if (!callSucceeded(api_->stop(0, axis, 0), "IMC_StopMove", axis, errorMessage)
        || !callSucceeded(api_->clearAxisStatus(0, axis), "IMC_ClrAxSts", axis, errorMessage)
        || !callSucceeded(api_->setCurrentPosition(0, axis, 0.0),
            "IMC_SetAxCurPos", axis, errorMessage)) {
        return false;
    }
    clock_->sleepMs(20);
    if (!callSucceeded(api_->syncPosition(0, axis), "IMC_SyncAxPos", axis, errorMessage)
        || !callSucceeded(api_->setCurrentPosition(0, axis, 0.0),
            "IMC_SetAxCurPos", axis, errorMessage)
        || !callSucceeded(api_->clearAxisStatus(0, axis),
            "IMC_ClrAxSts", axis, errorMessage)) {
        return false;
    }
    return true;
}

bool Imc60gMotionController::cleanupHardware(QString* errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!api_ || !cardOpened_) {
        return true;
    }

    QStringList failures;
    const auto recordFailure = [&](int code, const char* operation, short axis) {
        if (code != 0) {
            failures << formatApiFailure(operation, profile_.cardIndex, axis, code);
        }
    };

    recordFailure(api_->stop(0, 0, 1), "IMC_StopMove", 0);
    recordFailure(api_->stop(0, 1, 1), "IMC_StopMove", 1);
    if (servoEnabled_[0]) {
        recordFailure(api_->servoOff(0, 0), "IMC_ServoOff", 0);
    }
    if (servoEnabled_[1]) {
        recordFailure(api_->servoOff(0, 1), "IMC_ServoOff", 1);
    }
    if (ethercatTouched_) {
        recordFailure(api_->stopEthercat(0), "IMC_DelEcatComm", kNoAxis);
    }
    recordFailure(api_->closeCard(0), "IMC_CloseCard", kNoAxis);

    servoEnabled_[0] = false;
    servoEnabled_[1] = false;
    ethercatTouched_ = false;
    cardOpened_ = false;

    if (!failures.isEmpty()) {
        setError(errorMessage, failures.join("; "));
        return false;
    }
    return true;
}

void Imc60gMotionController::requestCancellation()
{
    cancelRequested_.store(true);
}

bool Imc60gMotionController::disconnect(QString* errorMessage)
{
    requestCancellation();
    QMutexLocker locker(&mutex_);
    if (errorMessage) {
        errorMessage->clear();
    }

    QString poisonReason;
    if (sdkIsPoisoned(&poisonReason)) {
        state_ = Imc60gConnectionState::Fault;
        setError(errorMessage,
            "IMC60G remains in Fault after an unverified shutdown; restart the process. "
            + poisonReason);
        releaseOwnership();
        return false;
    }

    QString cleanupError;
    if (!cleanupHardware(&cleanupError)) {
        poisonOwnership(cleanupError);
        state_ = Imc60gConnectionState::Fault;
        printActive_ = false;
        setError(errorMessage,
            "IMC60G shutdown is unverified; restart the process. " + cleanupError);
        return false;
    }

    releaseOwnership();
    printActive_ = false;
    state_ = Imc60gConnectionState::Disconnected;
    return true;
}

short Imc60gMotionController::physicalAxis(
    PrintHardwareProfile::LogicalAxis logicalAxis) const
{
    if (logicalAxis == PrintHardwareProfile::LogicalAxis::X) {
        return static_cast<short>(profile_.axisX);
    }
    if (logicalAxis == PrintHardwareProfile::LogicalAxis::Y) {
        return static_cast<short>(profile_.axisY);
    }
    return kNoAxis;
}

int Imc60gMotionController::homeDirection(
    PrintHardwareProfile::LogicalAxis logicalAxis) const
{
    return logicalAxis == PrintHardwareProfile::LogicalAxis::X
        ? profile_.homeDirectionX : profile_.homeDirectionY;
}

int Imc60gMotionController::homeBackoff(
    PrintHardwareProfile::LogicalAxis logicalAxis) const
{
    return logicalAxis == PrintHardwareProfile::LogicalAxis::X
        ? profile_.homeBackoffX : profile_.homeBackoffY;
}

bool Imc60gMotionController::moveRelative(
    PrintHardwareProfile::LogicalAxis logicalAxis, double millimeters,
    const PrintAxisConfig& axisConfig, QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    if (errorMessage) {
        errorMessage->clear();
    }
    const short axis = physicalAxis(logicalAxis);
    if (state_ != Imc60gConnectionState::Ready) {
        setError(errorMessage, "Manual IMC60G motion requires an explicit successful connectAndHome().");
        return false;
    }
    if (printActive_) {
        setError(errorMessage, "Manual IMC60G motion is blocked while a print is active.");
        return false;
    }
    if (axis < 0) {
        setError(errorMessage, "Manual IMC60G motion rejected: logical axis has no real physical axis.");
        return false;
    }
    if (!std::isfinite(millimeters) || millimeters == 0.0
        || axisConfig.maxDistance <= 0.0
        || std::abs(millimeters) > axisConfig.maxDistance) {
        setError(errorMessage, "Manual IMC60G motion exceeds the configured maximum travel.");
        return false;
    }
    if (axisConfig.subdivision <= 0 || axisConfig.resolution <= 0
        || axisConfig.speedOfMovement <= 0 || axisConfig.acceleratedVelocity <= 0
        || axisConfig.startSpeed < 0 || axisConfig.stopSpeed < 0) {
        setError(errorMessage, "Manual IMC60G motion has invalid speed or unit configuration.");
        return false;
    }

    const double signedDistance = axisConfig.changeDirection ? -millimeters : millimeters;
    const double pulseValue =
        signedDistance * axisConfig.subdivision * axisConfig.resolution;
    if (!std::isfinite(pulseValue)
        || pulseValue > std::numeric_limits<int>::max()
        || pulseValue < std::numeric_limits<int>::min()) {
        setError(errorMessage, "Manual IMC60G target overflows the native 32-bit position.");
        return false;
    }
    int currentPosition = 0;
    if (!callSucceeded(api_->plannedPosition(0, axis, &currentPosition),
            "IMC_GetAxPrfPos32", axis, errorMessage)) {
        return false;
    }
    const qlonglong relativePulses = std::llround(pulseValue);
    const qlonglong target = static_cast<qlonglong>(currentPosition) + relativePulses;
    if (target > std::numeric_limits<int>::max()
        || target < std::numeric_limits<int>::min()) {
        setError(errorMessage, "Manual IMC60G target overflows the native 32-bit position.");
        return false;
    }
    if (!callSucceeded(api_->setMotionProfile(0, axis,
            axisConfig.speedOfMovement,
            axisConfig.acceleratedVelocity,
            axisConfig.acceleratedVelocity,
            axisConfig.startSpeed),
            "IMC_SetAxMvPara", axis, errorMessage)) {
        return false;
    }
    if (axisConfig.stopSpeed > 0
        && !callSucceeded(api_->setAxisEndVelocity(0, axis, axisConfig.stopSpeed),
            "IMC_SetAxEndVel", axis, errorMessage)) {
        return false;
    }
    return callSucceeded(api_->startPtp(0, axis, static_cast<int>(target)),
        "IMC_StartPtpMove", axis, errorMessage);
}

bool Imc60gMotionController::stopAxis(
    PrintHardwareProfile::LogicalAxis logicalAxis, QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    const short axis = physicalAxis(logicalAxis);
    if (state_ != Imc60gConnectionState::Ready || axis < 0) {
        setError(errorMessage, "Cannot stop an unavailable IMC60G axis before connectAndHome().");
        return false;
    }
    return callSucceeded(api_->stop(0, axis, 1), "IMC_StopMove", axis, errorMessage);
}

bool Imc60gMotionController::readSnapshot(
    PrintHardwareProfile::LogicalAxis logicalAxis, Imc60gAxisSnapshot* snapshot,
    QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    const short axis = physicalAxis(logicalAxis);
    if (!snapshot) {
        setError(errorMessage, "IMC60G snapshot destination is null.");
        return false;
    }
    if (state_ != Imc60gConnectionState::Ready || axis < 0) {
        setError(errorMessage, "Cannot read an unavailable IMC60G axis before connectAndHome().");
        return false;
    }
    Imc60gAxisSnapshot value;
    value.logicalAxis = logicalAxis;
    value.physicalAxis = axis;
    if (!callSucceeded(api_->axisStatus(0, axis, &value.status), "IMC_GetAxSts", axis, errorMessage)
        || !callSucceeded(api_->stopReason(0, axis, &value.stopReason), "IMC_GetAxStopReason", axis, errorMessage)
        || !callSucceeded(api_->plannedPosition(0, axis, &value.plannedPosition), "IMC_GetAxPrfPos32", axis, errorMessage)
        || !callSucceeded(api_->encoderPosition(0, axis, &value.encoderPosition), "IMC_GetAxEncPos32", axis, errorMessage)) {
        return false;
    }
    *snapshot = value;
    return true;
}

bool Imc60gMotionController::isReadyForPrint() const
{
    QMutexLocker locker(&mutex_);
    return state_ == Imc60gConnectionState::Ready && !printActive_;
}

void Imc60gMotionController::setPrintActive(bool active)
{
    QMutexLocker locker(&mutex_);
    printActive_ = active;
}

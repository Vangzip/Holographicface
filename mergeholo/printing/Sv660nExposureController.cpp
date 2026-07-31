#include "Sv660nExposureController.h"

#include "IImc60gApi.h"
#include "PrintHardwareProfile.h"

#include <QThread>

#include <cstring>
#include <limits>
#include <utility>

namespace {

QString hexValue(quint64 value, int width)
{
    return QString("0x%1").arg(value, width, 16, QLatin1Char('0')).toUpper();
}

void setError(QString* errorMessage, const QString& value)
{
    if (errorMessage) {
        *errorMessage = value;
    }
}

} // namespace

Sv660nExposureController::Sv660nExposureController(IImc60gApi& api,
    const PrintHardwareProfile& profile, WaitFunction waitFunction)
    : api_(api)
    , profile_(profile)
    , waitFunction_(std::move(waitFunction))
{
    if (!waitFunction_) {
        waitFunction_ = [](int milliseconds) {
            QThread::msleep(static_cast<unsigned long>(milliseconds));
        };
    }
}

PrintExposureReadiness Sv660nExposureController::printReadiness(QString* errorMessage) const
{
    PrintExposureReadiness readiness;
    QString detail;
    readiness.profileMatches = validateProfile(&detail);
    readiness.safeBaseline = readiness.profileMatches && !armed_;
    if (errorMessage) *errorMessage = readiness.profileMatches ? QString() : detail;
    return readiness;
}

bool Sv660nExposureController::arm(qint32 begin, qint32 end, QString* errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }
    // An arm attempt crosses the hardware safety boundary. Until H18.00=0
    // has been explicitly acknowledged, retain the conservative unsafe state.
    armed_ = true;

    QString error;
    if (!validateProfile(&error)) {
        setError(errorMessage, error);
        return false;
    }

    int currentPosition = 0;
    const int positionRc =
        api_.plannedPosition(profile_.cardIndex,
            static_cast<short>(profile_.axisY), &currentPosition);
    if (positionRc != 0) {
        error = QString(
            "SV660N plannedPosition failed: card=%1 axis=%2 return=%3")
                    .arg(profile_.cardIndex)
                    .arg(profile_.axisY)
                    .arg(hexValue(static_cast<quint32>(positionRc), 8));
        return failArm(error, errorMessage);
    }

    qint64 low = static_cast<qint64>(begin);
    qint64 high = static_cast<qint64>(end);
    if (low > high) {
        std::swap(low, high);
    }

    const qint64 current = currentPosition;
    qint64 target = low;
    if (current <= low) {
        target = low;
    } else if (current >= high) {
        target = high;
    } else {
        const qint64 distanceToLow = current - low;
        const qint64 distanceToHigh = high - current;
        target = distanceToLow <= distanceToHigh ? low : high;
    }

    const qint64 relativeTarget = target - current;
    if (relativeTarget < std::numeric_limits<qint32>::min()
        || relativeTarget > std::numeric_limits<qint32>::max()) {
        error = QString(
            "SV660N relative target is outside int32: axis=%1 current=%2 "
            "target=%3 relative=%4")
                    .arg(profile_.axisY)
                    .arg(current)
                    .arg(target)
                    .arg(relativeTarget);
        return failArm(error, errorMessage);
    }

    if (profile_.sv660nPointIndex < 1 || profile_.sv660nPointIndex > 30) {
        error = QString("SV660N comparison point must be in [1,30]: point=%1")
                    .arg(profile_.sv660nPointIndex);
        return failArm(error, errorMessage);
    }

    const unsigned short point =
        static_cast<unsigned short>(profile_.sv660nPointIndex);
    const unsigned short targetSubIndex =
        static_cast<unsigned short>(1 + (point - 1) * 3);
    const unsigned short attributeSubIndex =
        static_cast<unsigned short>(3 + (point - 1) * 3);
    const qint32 targetValue = static_cast<qint32>(relativeTarget);
    const unsigned short attribute = static_cast<unsigned short>(
        targetValue >= 0 ? profile_.sv660nPositiveAttribute
                         : profile_.sv660nNegativeAttribute);

    auto requireWrite = [&](bool ok) {
        if (!ok) {
            return failArm(error, errorMessage);
        }
        return true;
    };

    if (!requireWrite(writeU16(0x2004, 0x01,
            static_cast<unsigned short>(profile_.sv660nDoFunction),
            "H04.00 DO1 function", &error))) {
        return false;
    }
    if (!requireWrite(writeU16(
            0x2018, 0x01, 0, "H18.00 compare enable off", &error))) {
        return false;
    }
    armed_ = false;
    if (!requireWrite(writeU16(0x2018, 0x13,
            profile_.sv660nUserUnits ? 256 : 0,
            "H18.18 H19 user-unit target", &error))) {
        return false;
    }
    if (!requireWrite(writeS32(
            0x2018, 0x0D, 0, "H18.12 compare zero offset", &error))) {
        return false;
    }
    if (!requireWrite(writeU16(
            0x2018, 0x05, 0, "H18.04 current-as-zero reset", &error))) {
        return false;
    }
    if (!requireWrite(writeU16(
            0x2018, 0x05, 1, "H18.04 current-as-zero", &error))) {
        return false;
    }
    waitFunction_(5);
    if (!requireWrite(writeU16(
            0x2018, 0x05, 0, "H18.04 current-as-zero release", &error))) {
        return false;
    }
    if (!requireWrite(writeU16(0x2018, 0x04,
            static_cast<unsigned short>(profile_.sv660nMode),
            "H18.03 compare mode", &error))) {
        return false;
    }
    if (!requireWrite(writeU16(0x2018, 0x06,
            static_cast<unsigned short>(profile_.sv660nWidth),
            "H18.05 output width", &error))) {
        return false;
    }
    if (!requireWrite(writeU16(
            0x2018, 0x08, point, "H18.07 start point", &error))) {
        return false;
    }
    if (!requireWrite(writeU16(
            0x2018, 0x09, point, "H18.08 end point", &error))) {
        return false;
    }
    if (!requireWrite(writeS32(0x2019, targetSubIndex,
            targetValue, "H19 target value", &error))) {
        return false;
    }
    if (!requireWrite(writeU16(0x2019, attributeSubIndex,
            attribute, "H19 attribute value", &error))) {
        return false;
    }

    auto requireVerification = [&](bool ok) {
        if (!ok) {
            return failArm(error, errorMessage);
        }
        return true;
    };

    if (!requireVerification(verifyU16(0x2004, 0x01,
            static_cast<unsigned short>(profile_.sv660nDoFunction),
            "H04.00 DO1 function", &error))) {
        return false;
    }
    if (!requireVerification(verifyU16(0x2018, 0x13,
            profile_.sv660nUserUnits ? 256 : 0,
            "H18.18 H19 user-unit target", &error))) {
        return false;
    }
    if (!requireVerification(verifyS32(
            0x2018, 0x0D, 0, "H18.12 compare zero offset", &error))) {
        return false;
    }
    if (!requireVerification(verifyU16(0x2018, 0x04,
            static_cast<unsigned short>(profile_.sv660nMode),
            "H18.03 compare mode", &error))) {
        return false;
    }
    if (!requireVerification(verifyU16(0x2018, 0x06,
            static_cast<unsigned short>(profile_.sv660nWidth),
            "H18.05 output width", &error))) {
        return false;
    }
    if (!requireVerification(verifyU16(
            0x2018, 0x08, point, "H18.07 start point", &error))) {
        return false;
    }
    if (!requireVerification(verifyU16(
            0x2018, 0x09, point, "H18.08 end point", &error))) {
        return false;
    }
    if (!requireVerification(verifyS32(0x2019, targetSubIndex,
            targetValue, "H19 target value", &error))) {
        return false;
    }
    if (!requireVerification(verifyU16(0x2019, attributeSubIndex,
            attribute, "H19 attribute value", &error))) {
        return false;
    }

    // A failed enable call may still have reached the drive. Mark unsafe
    // before issuing it; failArm() will clear this only after disable succeeds.
    armed_ = true;
    if (!requireWrite(writeU16(
            0x2018, 0x01, 1, "H18.00 compare enable on", &error))) {
        return false;
    }

    armed_ = true;
    return true;
}

bool Sv660nExposureController::disarm(QString* errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    QString error;
    if (!validateProfile(&error)) {
        armed_ = true;
        setError(errorMessage, error);
        return false;
    }
    if (!writeU16(
            0x2018, 0x01, 0, "H18.00 compare enable off", &error)) {
        armed_ = true;
        setError(errorMessage, error);
        return false;
    }

    armed_ = false;
    return true;
}

bool Sv660nExposureController::isArmed() const
{
    return armed_;
}

bool Sv660nExposureController::writeU16(unsigned short index,
    unsigned short subIndex, unsigned short value, const QString& operation,
    QString* errorMessage)
{
    unsigned short encoded = value;
    unsigned int abortCode = 0;
    const int rc = api_.setAxisSdo(profile_.cardIndex,
        static_cast<short>(profile_.axisY), index, subIndex,
        reinterpret_cast<unsigned char*>(&encoded), sizeof(encoded), &abortCode);
    if (rc == 0 && abortCode == 0) {
        return true;
    }

    setError(errorMessage,
        QString(
            "SV660N SDO write failed: operation=%1 axis=%2 index=%3 sub=%4 "
            "value=%5 return=%6 abort=%7")
            .arg(operation)
            .arg(profile_.axisY)
            .arg(hexValue(index, 4))
            .arg(hexValue(subIndex, 2))
            .arg(value)
            .arg(hexValue(static_cast<quint32>(rc), 8))
            .arg(hexValue(abortCode, 8)));
    return false;
}

bool Sv660nExposureController::writeS32(unsigned short index,
    unsigned short subIndex, qint32 value, const QString& operation,
    QString* errorMessage)
{
    qint32 encoded = value;
    unsigned int abortCode = 0;
    const int rc = api_.setAxisSdo(profile_.cardIndex,
        static_cast<short>(profile_.axisY), index, subIndex,
        reinterpret_cast<unsigned char*>(&encoded), sizeof(encoded), &abortCode);
    if (rc == 0 && abortCode == 0) {
        return true;
    }

    setError(errorMessage,
        QString(
            "SV660N SDO write failed: operation=%1 axis=%2 index=%3 sub=%4 "
            "value=%5 return=%6 abort=%7")
            .arg(operation)
            .arg(profile_.axisY)
            .arg(hexValue(index, 4))
            .arg(hexValue(subIndex, 2))
            .arg(value)
            .arg(hexValue(static_cast<quint32>(rc), 8))
            .arg(hexValue(abortCode, 8)));
    return false;
}

bool Sv660nExposureController::verifyU16(unsigned short index,
    unsigned short subIndex, unsigned short expected, const QString& operation,
    QString* errorMessage)
{
    unsigned short actual = 0;
    unsigned int resultSize = 0;
    unsigned int abortCode = 0;
    const int rc = api_.getAxisSdo(profile_.cardIndex,
        static_cast<short>(profile_.axisY), index, subIndex,
        reinterpret_cast<unsigned char*>(&actual), sizeof(actual),
        &resultSize, &abortCode);
    if (rc != 0 || abortCode != 0) {
        setError(errorMessage,
            QString(
                "SV660N SDO read failed: operation=%1 axis=%2 index=%3 sub=%4 "
                "expected=%5 resultSize=%6 return=%7 abort=%8")
                .arg(operation)
                .arg(profile_.axisY)
                .arg(hexValue(index, 4))
                .arg(hexValue(subIndex, 2))
                .arg(expected)
                .arg(resultSize)
                .arg(hexValue(static_cast<quint32>(rc), 8))
                .arg(hexValue(abortCode, 8)));
        return false;
    }
    if (resultSize != sizeof(actual)) {
        setError(errorMessage,
            QString(
                "SV660N SDO read size mismatch: operation=%1 axis=%2 index=%3 "
                "sub=%4 expectedSize=%5 actualSize=%6 return=%7 abort=%8")
                .arg(operation)
                .arg(profile_.axisY)
                .arg(hexValue(index, 4))
                .arg(hexValue(subIndex, 2))
                .arg(sizeof(actual))
                .arg(resultSize)
                .arg(hexValue(static_cast<quint32>(rc), 8))
                .arg(hexValue(abortCode, 8)));
        return false;
    }
    if (actual != expected) {
        setError(errorMessage,
            QString(
                "SV660N SDO read value mismatch: operation=%1 axis=%2 index=%3 "
                "sub=%4 expected=%5 actual=%6 return=%7 abort=%8")
                .arg(operation)
                .arg(profile_.axisY)
                .arg(hexValue(index, 4))
                .arg(hexValue(subIndex, 2))
                .arg(expected)
                .arg(actual)
                .arg(hexValue(static_cast<quint32>(rc), 8))
                .arg(hexValue(abortCode, 8)));
        return false;
    }
    return true;
}

bool Sv660nExposureController::verifyS32(unsigned short index,
    unsigned short subIndex, qint32 expected, const QString& operation,
    QString* errorMessage)
{
    qint32 actual = 0;
    unsigned int resultSize = 0;
    unsigned int abortCode = 0;
    const int rc = api_.getAxisSdo(profile_.cardIndex,
        static_cast<short>(profile_.axisY), index, subIndex,
        reinterpret_cast<unsigned char*>(&actual), sizeof(actual),
        &resultSize, &abortCode);
    if (rc != 0 || abortCode != 0) {
        setError(errorMessage,
            QString(
                "SV660N SDO read failed: operation=%1 axis=%2 index=%3 sub=%4 "
                "expected=%5 resultSize=%6 return=%7 abort=%8")
                .arg(operation)
                .arg(profile_.axisY)
                .arg(hexValue(index, 4))
                .arg(hexValue(subIndex, 2))
                .arg(expected)
                .arg(resultSize)
                .arg(hexValue(static_cast<quint32>(rc), 8))
                .arg(hexValue(abortCode, 8)));
        return false;
    }
    if (resultSize != sizeof(actual)) {
        setError(errorMessage,
            QString(
                "SV660N SDO read size mismatch: operation=%1 axis=%2 index=%3 "
                "sub=%4 expectedSize=%5 actualSize=%6 return=%7 abort=%8")
                .arg(operation)
                .arg(profile_.axisY)
                .arg(hexValue(index, 4))
                .arg(hexValue(subIndex, 2))
                .arg(sizeof(actual))
                .arg(resultSize)
                .arg(hexValue(static_cast<quint32>(rc), 8))
                .arg(hexValue(abortCode, 8)));
        return false;
    }
    if (actual != expected) {
        setError(errorMessage,
            QString(
                "SV660N SDO read value mismatch: operation=%1 axis=%2 index=%3 "
                "sub=%4 expected=%5 actual=%6 return=%7 abort=%8")
                .arg(operation)
                .arg(profile_.axisY)
                .arg(hexValue(index, 4))
                .arg(hexValue(subIndex, 2))
                .arg(expected)
                .arg(actual)
                .arg(hexValue(static_cast<quint32>(rc), 8))
                .arg(hexValue(abortCode, 8)));
        return false;
    }
    return true;
}

bool Sv660nExposureController::failArm(
    const QString& primaryError, QString* errorMessage)
{
    armed_ = true;
    QString cleanupError;
    if (!writeU16(0x2018, 0x01, 0,
            "H18.00 compare enable off cleanup", &cleanupError)) {
        setError(errorMessage,
            primaryError + "; cleanup also failed: " + cleanupError);
    } else {
        armed_ = false;
        setError(errorMessage, primaryError);
    }
    return false;
}

bool Sv660nExposureController::validateProfile(QString* errorMessage) const
{
    QString validationError;
    if (validatePrintHardwareProfile(profile_, &validationError)) {
        return true;
    }
    setError(errorMessage,
        QString("Invalid IMC60G production profile for SV660N exposure: %1")
            .arg(validationError));
    return false;
}

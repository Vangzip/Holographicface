#pragma once

#include "PrintHardwareProfile.h"

#include <QString>

#include <functional>

class IImc60gApi;

class Sv660nExposureController {
public:
    using WaitFunction = std::function<void(int milliseconds)>;

    Sv660nExposureController(IImc60gApi& api,
        const PrintHardwareProfile& profile,
        WaitFunction waitFunction = WaitFunction());

    bool arm(long begin, long end, QString* errorMessage = nullptr);
    bool disarm(QString* errorMessage = nullptr);
    bool isArmed() const;

private:
    bool writeU16(unsigned short index, unsigned short subIndex,
        unsigned short value, const QString& operation, QString* errorMessage);
    bool writeS32(unsigned short index, unsigned short subIndex,
        qint32 value, const QString& operation, QString* errorMessage);
    bool verifyU16(unsigned short index, unsigned short subIndex,
        unsigned short expected, const QString& operation, QString* errorMessage);
    bool verifyS32(unsigned short index, unsigned short subIndex,
        qint32 expected, const QString& operation, QString* errorMessage);
    bool failArm(const QString& primaryError, QString* errorMessage);
    bool validateProfile(QString* errorMessage) const;

    IImc60gApi& api_;
    PrintHardwareProfile profile_;
    WaitFunction waitFunction_;
    bool armed_ = false;
};

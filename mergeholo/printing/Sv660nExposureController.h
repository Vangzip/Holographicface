#pragma once

#include "PrintHardwareProfile.h"
#include "IExposureController.h"

#include <QString>

#include <functional>

class IImc60gApi;

class Sv660nExposureController final : public IExposureController {
public:
    using WaitFunction = std::function<void(int milliseconds)>;

    Sv660nExposureController(IImc60gApi& api,
        const PrintHardwareProfile& profile,
        WaitFunction waitFunction = WaitFunction());

    PrintExposureReadiness printReadiness(QString* errorMessage = nullptr) const override;
    bool arm(qint32 begin, qint32 end, QString* errorMessage = nullptr) override;
    bool disarm(QString* errorMessage = nullptr) override;
    bool isArmed() const override;

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

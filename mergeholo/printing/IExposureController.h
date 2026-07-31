#pragma once

#include <QString>
#include <QtGlobal>

struct PrintExposureReadiness {
    bool profileMatches = false;
    bool safeBaseline = false;
};

class IExposureController
{
public:
    virtual ~IExposureController() = default;

    virtual PrintExposureReadiness printReadiness(QString* errorMessage = nullptr) const = 0;
    virtual bool arm(qint32 begin, qint32 end, QString* errorMessage = nullptr) = 0;
    virtual bool disarm(QString* errorMessage = nullptr) = 0;
    virtual bool isArmed() const = 0;
};

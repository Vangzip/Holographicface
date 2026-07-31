#pragma once

#include <QString>
#include <QtGlobal>

struct VBlankDiagnostics {
    qint32 hresult = static_cast<qint32>(0x80004001u);
    QString outputDeviceName;
    int adapterIndex = -1;
    int outputIndex = -1;
};

class IVBlankWaiter
{
public:
    virtual ~IVBlankWaiter() = default;
    virtual bool waitForPhysicalVBlank(VBlankDiagnostics* diagnostics,
        QString* errorMessage = nullptr) = 0;
};

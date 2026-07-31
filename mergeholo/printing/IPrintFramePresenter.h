#pragma once

#include "PrintFrame.h"

#include <QSize>
#include <QString>
#include <QtGlobal>

struct PrintPresenterReadiness {
    bool secondScreenAttached = false;
    bool presenterAvailable = false;
    bool vblankReady = false;
    bool generationCurrent = false;
};

struct PrintRowVBlankAnchor {
    quint64 presenterGeneration = 0;
    bool available = false;
};

class IPrintFramePresenter
{
public:
    virtual ~IPrintFramePresenter() = default;

    virtual PrintPresenterReadiness printReadiness(QString* errorMessage = nullptr) const = 0;
    virtual bool prepare(const PrintFrame& firstFrame, const QSize& targetSize, QString* errorMessage = nullptr) = 0;
    virtual bool present(const PrintFrame& frame, const QSize& targetSize, QString* errorMessage = nullptr) = 0;
    virtual bool presentRowAnchor(const PrintFrame& frame, const QSize& targetSize,
        QString* errorMessage = nullptr)
    {
        return present(frame, targetSize, errorMessage);
    }
    virtual bool waitForDisplayFrame(QString* errorMessage = nullptr) = 0;
    virtual bool acquireRowAnchor(PrintRowVBlankAnchor* anchor,
        QString* errorMessage = nullptr)
    {
        if (!anchor || !waitForDisplayFrame(errorMessage)) return false;
        anchor->available = true;
        return true;
    }
    virtual bool waitForRowSlot(int slot, PrintRowVBlankAnchor* anchor,
        QString* errorMessage = nullptr)
    {
        if (slot < 0 || !anchor || !anchor->available) return false;
        if (slot == 0) {
            anchor->available = false;
            return true;
        }
        return waitForDisplayFrame(errorMessage);
    }
    bool waitForVBlank(QString* errorMessage = nullptr) { return waitForDisplayFrame(errorMessage); }
    virtual void shutdown() = 0;
    virtual bool shutdownChecked(QString* errorMessage = nullptr)
    {
        shutdown();
        if (errorMessage) errorMessage->clear();
        return true;
    }
};

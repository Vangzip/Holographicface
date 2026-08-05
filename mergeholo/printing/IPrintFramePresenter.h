#pragma once

#include "PrintFrame.h"

#include <QSize>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include <limits>

struct PrintPresenterReadiness {
    bool secondScreenAttached = false;
    bool presenterAvailable = false;
    bool vblankReady = false;
    bool generationCurrent = false;
};

struct PrintRowVBlankAnchor {
    quint64 presenterGeneration = 0;
    qint64 qpcUs = 0;
    bool available = false;
};

struct PrintRowVBlankCounterSnapshot {
    bool prepared = false;
    bool counting = false;
    qint32 frame = -1;
    qint32 frameSnapshot = 0;
    qint64 frameZeroQpcUs = 0;
    qint64 lastVBlankQpcUs = 0;
    qint32 lastWaitHresult = static_cast<qint32>(0x8000000Au);
    quint32 adapterIndex = std::numeric_limits<quint32>::max();
    quint32 outputIndex = std::numeric_limits<quint32>::max();
};

// This is sampled only after Present has returned and after the independent
// RowVBlankCounter snapshot.  It mirrors V2's DXGI_FRAME_STATISTICS_MEDIA log
// without participating in the VBlank-to-Present path.
struct PrintFrameStatisticsMedia {
    qint32 hresult = static_cast<qint32>(0x8000000Au);
    quint32 presentCount = 0;
    quint32 presentRefreshCount = 0;
    quint32 syncRefreshCount = 0;
    qint64 syncQpcUs = -1;
    quint32 compositionMode = std::numeric_limits<quint32>::max();
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
    virtual bool prepareRow(const QVector<PrintFrame>& rowFrames,
        const QVector<QSize>& targetSizes, QString* errorMessage = nullptr)
    {
        Q_UNUSED(rowFrames);
        Q_UNUSED(targetSizes);
        if (errorMessage)
            *errorMessage = QStringLiteral("Prepared-row presentation is unsupported by this presenter.");
        return false;
    }
    virtual bool presentPreparedRowFrame(int logicalFrame,
        QString* errorMessage = nullptr)
    {
        Q_UNUSED(logicalFrame);
        if (errorMessage)
            *errorMessage = QStringLiteral("Prepared-row presentation is unsupported by this presenter.");
        return false;
    }
    virtual void clearPreparedRow() {}
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
    // V2 samples swap-chain frame statistics after every physical VBlank and
    // immediately before Present.  It is telemetry-only: a failed query must
    // never delay or abort the paced submission path.
    virtual void sampleFrameStatistics() {}
    virtual PrintFrameStatisticsMedia sampleFrameStatisticsMedia() { return {}; }
    virtual bool prepareRowVBlankCounter(QString* errorMessage = nullptr)
    {
        if (errorMessage) errorMessage->clear();
        return false;
    }
    virtual void beginRowVBlankCounter(qint64) {}
    virtual PrintRowVBlankCounterSnapshot rowVBlankCounterSnapshot() const { return {}; }
    virtual void stopRowVBlankCounter() {}
    bool waitForVBlank(QString* errorMessage = nullptr) { return waitForDisplayFrame(errorMessage); }
    virtual void shutdown() = 0;
    virtual bool shutdownChecked(QString* errorMessage = nullptr)
    {
        shutdown();
        if (errorMessage) errorMessage->clear();
        return true;
    }
};

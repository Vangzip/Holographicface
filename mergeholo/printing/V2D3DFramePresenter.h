#pragma once

#include "IPrintFramePresenter.h"
#include "IVBlankWaiter.h"
#include "SecondScreenSelection.h"

#include <QByteArray>
#include <QMutex>
#include <QStringList>

#include <functional>
#include <memory>

struct PresenterDiagnostics {
    bool ready = false;
    QString selectedOutputDeviceName;
    qint32 prepareHresult = static_cast<qint32>(0x80004001u);
    qint32 uploadHresult = static_cast<qint32>(0x80004001u);
    qint32 presentHresult = static_cast<qint32>(0x80004001u);
    qint32 frameStatisticsHresult = static_cast<qint32>(0x80004001u);
    qint32 frameStatisticsMediaHresult = static_cast<qint32>(0x80004001u);
    quint32 presentCount = 0;
    quint32 presentRefreshCount = 0;
    quint32 syncRefreshCount = 0;
    qint64 frameLatencyWaitResult = -1;
    quint64 orderedPresentCount = 0;
    VBlankDiagnostics vblank;
};

class IPresentationDispatcher
{
public:
    virtual ~IPresentationDispatcher() = default;
    virtual bool invokeSynchronously(const std::function<bool()>& command,
        QString* errorMessage = nullptr) = 0;
};

class IV2D3DBackend
{
public:
    virtual ~IV2D3DBackend() = default;
    virtual bool prepare(const DisplayMonitor& monitor, const QSize& surfaceSize,
        PresenterDiagnostics* diagnostics, QString* errorMessage = nullptr) = 0;
    virtual bool outputMatches(const DisplayMonitor& monitor,
        QString* errorMessage = nullptr) const = 0;
    virtual bool uploadAndPresent(const QByteArray& packedBgra, int width, int height,
        int rowPitch, const QSize& targetSize, PresenterDiagnostics* diagnostics,
        QString* errorMessage = nullptr) = 0;
    virtual void shutdown() = 0;
};

struct V2RowVBlankAnchor {
private:
    friend class V2D3DFramePresenter;
    quint64 presenterGeneration = 0;
    bool available = false;
};

class V2D3DFramePresenter final : public IPrintFramePresenter
{
public:
    V2D3DFramePresenter();
    V2D3DFramePresenter(QVector<DisplayMonitor> displays,
        std::shared_ptr<IV2D3DBackend> backend,
        std::shared_ptr<IVBlankWaiter> vblankWaiter,
        std::shared_ptr<IPresentationDispatcher> dispatcher);
    ~V2D3DFramePresenter() override;

    bool prepare(const PrintFrame& firstFrame, const QSize& targetSize,
        QString* errorMessage = nullptr) override;
    bool present(const PrintFrame& frame, const QSize& targetSize,
        QString* errorMessage = nullptr) override;
    bool waitForDisplayFrame(QString* errorMessage = nullptr) override;
    bool acquireRowAnchor(V2RowVBlankAnchor* anchor, QString* errorMessage = nullptr);
    bool waitForRowSlot(int slot, V2RowVBlankAnchor* anchor,
        QString* errorMessage = nullptr);
    void shutdown() override;

    bool isReady() const;
    PresenterDiagnostics diagnostics() const;

private:
    bool convertFrame(const PrintFrame& frame, const QSize& targetSize,
        QByteArray* packedBgra, QString* errorMessage) const;
    bool waitForPhysicalVBlankLocked(QString* errorMessage);
    void shutdownLocked();

    QVector<DisplayMonitor> displays_;
    std::shared_ptr<IV2D3DBackend> backend_;
    std::shared_ptr<IVBlankWaiter> vblankWaiter_;
    std::shared_ptr<IPresentationDispatcher> dispatcher_;
    mutable QMutex mutex_;
    PresenterDiagnostics diagnostics_;
    DisplayMonitor selectedMonitor_;
    bool backendActive_ = false;
    bool ready_ = false;
    quint64 generation_ = 1;
};

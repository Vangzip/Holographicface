#pragma once

#include "IPrintFramePresenter.h"
#include "IVBlankWaiter.h"
#include "SecondScreenSelection.h"

#include <QByteArray>
#include <QMutex>
#include <QStringList>

#include <functional>
#include <memory>

class V2RowVBlankCounter;

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
    QString dispatcherFailure;
    VBlankDiagnostics vblank;
};

class IPresentationDispatcher
{
public:
    virtual ~IPresentationDispatcher() = default;
    // Runs the command synchronously on one GUI/presentation owner. The caller
    // must not hold presenter state locks while invoking this method.
    virtual bool invokeSynchronously(const std::function<bool()>& command,
        QString* errorMessage = nullptr) = 0;
};

struct V2PreparedRowFrame {
    QByteArray packedBgra;
    int width = 0;
    int height = 0;
    int rowPitch = 0;
    QSize targetSize;
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
    virtual bool prepareRow(const QVector<V2PreparedRowFrame>& frames,
        PresenterDiagnostics* diagnostics, QString* errorMessage = nullptr) = 0;
    virtual bool presentPreparedRowFrame(int cacheIndex,
        PresenterDiagnostics* diagnostics, QString* errorMessage = nullptr) = 0;
    virtual void sampleFrameStatistics(PresenterDiagnostics*) {}
    virtual void sampleFrameStatisticsMedia(PrintFrameStatisticsMedia*) {}
    virtual void clearPreparedRow() = 0;
    virtual void shutdown() = 0;
};

using V2RowVBlankAnchor = PrintRowVBlankAnchor;

class V2D3DFramePresenter final : public IPrintFramePresenter
{
public:
    V2D3DFramePresenter();
    V2D3DFramePresenter(QVector<DisplayMonitor> displays,
        std::shared_ptr<IV2D3DBackend> backend,
        std::shared_ptr<IVBlankWaiter> vblankWaiter,
        std::shared_ptr<IPresentationDispatcher> dispatcher);
    ~V2D3DFramePresenter() override;

    PrintPresenterReadiness printReadiness(QString* errorMessage = nullptr) const override;
    bool prepare(const PrintFrame& firstFrame, const QSize& targetSize,
        QString* errorMessage = nullptr) override;
    bool present(const PrintFrame& frame, const QSize& targetSize,
        QString* errorMessage = nullptr) override;
    bool presentRowAnchor(const PrintFrame& frame, const QSize& targetSize,
        QString* errorMessage = nullptr) override;
    bool prepareRow(const QVector<PrintFrame>& rowFrames,
        const QVector<QSize>& targetSizes, QString* errorMessage = nullptr) override;
    bool presentPreparedRowFrame(int logicalFrame,
        QString* errorMessage = nullptr) override;
    void clearPreparedRow() override;
    bool waitForDisplayFrame(QString* errorMessage = nullptr) override;
    bool acquireRowAnchor(V2RowVBlankAnchor* anchor, QString* errorMessage = nullptr) override;
    bool waitForRowSlot(int slot, V2RowVBlankAnchor* anchor,
        QString* errorMessage = nullptr) override;
    void sampleFrameStatistics() override;
    PrintFrameStatisticsMedia sampleFrameStatisticsMedia() override;
    bool prepareRowVBlankCounter(QString* errorMessage = nullptr) override;
    void beginRowVBlankCounter(qint64 frameZeroQpcUs) override;
    PrintRowVBlankCounterSnapshot rowVBlankCounterSnapshot() const override;
    void stopRowVBlankCounter() override;
    void shutdown() override;

    bool refreshAttachedDisplays(QString* errorMessage = nullptr);
    bool isReady() const;
    double selectedRefreshHz(QString* errorMessage = nullptr) const;
    PresenterDiagnostics diagnostics() const;

private:
    bool convertFrame(const PrintFrame& frame, const QSize& targetSize,
        QByteArray* packedBgra, QString* errorMessage) const;
    void failDispatcherCommand(quint64 expectedGeneration, const QString& commandName,
        QString* errorMessage);
    void invalidateReadyState();

    QVector<DisplayMonitor> displays_;
    std::shared_ptr<IV2D3DBackend> backend_;
    std::shared_ptr<IVBlankWaiter> vblankWaiter_;
    std::shared_ptr<IPresentationDispatcher> dispatcher_;
    // D3D11's immediate context may move between threads, but it must have one
    // caller at a time. Startup/shutdown use the GUI thread; the V2 cached path
    // uses the high-priority row display thread directly.
    mutable QMutex rendererMutex_;
    mutable QMutex stateMutex_;
    PresenterDiagnostics diagnostics_;
    DisplayMonitor selectedMonitor_;
    bool backendActive_ = false;
    bool ready_ = false;
    int preparedRowSize_ = 0;
    quint64 preparedRowGeneration_ = 1;
    quint64 generation_ = 1;
    std::unique_ptr<V2RowVBlankCounter> rowVBlankCounter_;
};

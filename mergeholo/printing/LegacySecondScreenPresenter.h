#pragma once

#include "IPrintFramePresenter.h"

#include <QObject>
#include <QRect>

#include <functional>
#include <memory>

class LegacyD3DImageRenderer;
class QWidget;

class LegacySecondScreenPresenter final : public QObject, public IPrintFramePresenter
{
public:
    explicit LegacySecondScreenPresenter(QObject* parent = nullptr);
    ~LegacySecondScreenPresenter() override;

    bool prepare(const PrintFrame& firstFrame, const QSize& targetSize, QString* errorMessage = nullptr) override;
    bool present(const PrintFrame& frame, const QSize& targetSize, QString* errorMessage = nullptr) override;
    bool waitForVBlank(QString* errorMessage = nullptr) override;
    void shutdown() override;

private:
    bool invokeOnPresenterThread(
        const std::function<bool(QString*)>& action,
        QString* errorMessage);
    bool prepareOnPresenterThread(const PrintFrame& firstFrame, const QSize& targetSize, QString* errorMessage);
    bool presentOnPresenterThread(const PrintFrame& frame, const QSize& targetSize, QString* errorMessage);
    bool waitForVBlankOnPresenterThread(QString* errorMessage);
    void shutdownOnPresenterThread();
    bool resizeTargetWindow(const QSize& targetSize, QString* errorMessage);

    QRect displayGeometry_;
    QSize targetSize_;
    std::unique_ptr<QWidget> displayWindow_;
    std::unique_ptr<QWidget> imageWindow_;
    std::unique_ptr<LegacyD3DImageRenderer> renderer_;
};

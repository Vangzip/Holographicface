#pragma once

#include "LightFieldCapture.h"

#include <QElapsedTimer>
#include <QImage>
#include <QMainWindow>
#include <QString>
#include <QThread>

#include <atomic>
#include <memory>

#include <opencv2/core.hpp>

class QLabel;
class QPushButton;
class QProgressBar;
class QTimer;

namespace Ui {
class CaptureWindow;
}

class CaptureWindow : public QMainWindow
{
public:
    CaptureWindow(const QString& projectRoot, const QString& cameraConfigPath, QWidget* parent = nullptr);
    ~CaptureWindow() override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    enum class State {
        Starting,
        Live,
        Frozen,
        Processing,
        Done,
        Error
    };

    void buildUi();
    void startCamera();
    void releaseCamera();
    void pollCameraFrame();
    void captureFrame();
    void resetCapture();
    void startProcessing();
    bool preparePipelineInput(QString* errorMessage);
    bool writePipelineConfig(QString* errorMessage);
    void startPipelineThread();
    void finishPipelineRun(int exitCode, bool normalExit);
    void setState(State state);
    void setProgress(int value, const QString& text);
    void setPreviewImages(const cv::Mat& rgb, const cv::Mat& depthDisplay);
    void renderPreviews();
    void renderLabel(QLabel* label, const QImage& image, const QString& fallbackText);
    QString outputRoot() const;
    QString inputRoot() const;
    QString pipelineConfigPath() const;
    QString pipelineLogPath() const;

    QString projectRoot_;
    QString cameraConfigPath_;
    std::unique_ptr<Ui::CaptureWindow> ui_;

    QLabel* rgbLabel_ = nullptr;
    QLabel* depthLabel_ = nullptr;
    QLabel* progressText_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QPushButton* captureButton_ = nullptr;
    QPushButton* confirmButton_ = nullptr;
    QPushButton* retakeButton_ = nullptr;
    QTimer* frameTimer_ = nullptr;
    QThread* pipelineThread_ = nullptr;
    std::atomic<int> pipelineExitCode_{0};
    std::atomic<bool> pipelineNormalExit_{true};

    std::unique_ptr<LightFieldCapture> capture_;
    State state_ = State::Starting;
    bool hasLiveFrame_ = false;

    cv::Mat latestRgb_;
    cv::Mat latestDepthForPipeline_;
    cv::Mat latestDepthDisplay_;
    cv::Mat frozenRgb_;
    cv::Mat frozenDepthForPipeline_;
    cv::Mat frozenDepthDisplay_;

    QImage rgbPreview_;
    QImage depthPreview_;
    QElapsedTimer confirmTimer_;
};

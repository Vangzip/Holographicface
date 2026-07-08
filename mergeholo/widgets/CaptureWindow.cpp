#include "CaptureWindow.h"

#include "HoloPipeline.h"
#include "ui_CaptureWindow.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QPixmap>
#include <QSizePolicy>
#include <QTextStream>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace {

QString cleanPath(const QString& path)
{
    return QDir::fromNativeSeparators(QFileInfo(path).absoluteFilePath());
}

bool removeDirectoryIfExists(const QString& path)
{
    QDir dir(path);
    return !dir.exists() || dir.removeRecursively();
}

QImage matToImage(const cv::Mat& input)
{
    if (input.empty()) {
        return {};
    }

    cv::Mat converted;
    if (input.type() == CV_8UC3) {
        cv::cvtColor(input, converted, cv::COLOR_BGR2RGB);
    }
    else if (input.type() == CV_8UC4) {
        cv::cvtColor(input, converted, cv::COLOR_BGRA2RGBA);
    }
    else if (input.type() == CV_8UC1) {
        converted = input;
    }
    else {
        cv::Mat gray;
        input.convertTo(gray, CV_8UC1);
        converted = gray;
    }

    if (converted.type() == CV_8UC3) {
        return QImage(converted.data, converted.cols, converted.rows,
            static_cast<int>(converted.step), QImage::Format_RGB888).copy();
    }
    if (converted.type() == CV_8UC4) {
        return QImage(converted.data, converted.cols, converted.rows,
            static_cast<int>(converted.step), QImage::Format_RGBA8888).copy();
    }
    if (converted.type() == CV_8UC1) {
        return QImage(converted.data, converted.cols, converted.rows,
            static_cast<int>(converted.step), QImage::Format_Grayscale8).copy();
    }
    return {};
}

cv::Mat finiteFloatToGray(const cv::Mat& input)
{
    cv::Mat source;
    if (input.channels() == 3) {
        std::vector<cv::Mat> channels;
        cv::split(input, channels);
        source = channels[2];
    }
    else {
        source = input;
    }

    cv::Mat floatSource;
    source.convertTo(floatSource, CV_32F);

    float minValue = std::numeric_limits<float>::max();
    float maxValue = -std::numeric_limits<float>::max();
    for (int y = 0; y < floatSource.rows; ++y) {
        const float* row = floatSource.ptr<float>(y);
        for (int x = 0; x < floatSource.cols; ++x) {
            const float value = row[x];
            if (!std::isfinite(value) || std::fabs(value) < 1e-6f) {
                continue;
            }
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
        }
    }

    cv::Mat gray(floatSource.rows, floatSource.cols, CV_8UC1, cv::Scalar(0));
    if (minValue >= maxValue) {
        return gray;
    }

    const float scale = 255.0f / (maxValue - minValue);
    for (int y = 0; y < floatSource.rows; ++y) {
        const float* src = floatSource.ptr<float>(y);
        unsigned char* dst = gray.ptr<unsigned char>(y);
        for (int x = 0; x < floatSource.cols; ++x) {
            const float value = src[x];
            if (!std::isfinite(value) || std::fabs(value) < 1e-6f) {
                dst[x] = 0;
                continue;
            }
            const float normalized = (value - minValue) * scale;
            dst[x] = static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, normalized)));
        }
    }
    return gray;
}

QImage depthToImage(const cv::Mat& input)
{
    if (input.empty()) {
        return {};
    }

    if (input.depth() == CV_32F || input.depth() == CV_64F) {
        cv::Mat gray = finiteFloatToGray(input);
        cv::Mat colored;
        cv::applyColorMap(gray, colored, cv::COLORMAP_JET);
        return matToImage(colored);
    }

    return matToImage(input);
}

QString forwardSlashes(const QString& path)
{
    return QDir::fromNativeSeparators(path);
}

} // namespace

CaptureWindow::CaptureWindow(const QString& projectRoot, const QString& cameraConfigPath, QWidget* parent)
    : QMainWindow(parent)
    , projectRoot_(QDir(projectRoot).absolutePath())
    , cameraConfigPath_(cameraConfigPath)
{
    buildUi();
    setState(State::Starting);
    QTimer::singleShot(0, this, [this] { startCamera(); });
}

CaptureWindow::~CaptureWindow()
{
    if (pipelineThread_) {
        QThread* thread = pipelineThread_;
        pipelineThread_ = nullptr;
        thread->requestInterruption();
        thread->wait();
        delete thread;
    }
    releaseCamera();
}

void CaptureWindow::buildUi()
{
    ui_.reset(new Ui::CaptureWindow);
    ui_->setupUi(this);

    rgbLabel_ = ui_->rgbLabel;
    depthLabel_ = ui_->depthLabel;
    progressText_ = ui_->progressText;
    progressBar_ = ui_->progressBar;
    captureButton_ = ui_->captureButton;
    confirmButton_ = ui_->confirmButton;
    retakeButton_ = ui_->retakeButton;

    const QList<QPushButton*> buttons = { captureButton_, confirmButton_, retakeButton_ };
    for (QPushButton* button : buttons) {
        button->setStyleSheet(
            "QPushButton { background: #ffffff; border: 2px solid #111111; font-size: 16px; }"
            "QPushButton:disabled { color: #888888; border-color: #888888; background: #f2f2f2; }");
    }

    frameTimer_ = new QTimer(this);
    frameTimer_->setInterval(30);
    connect(frameTimer_, &QTimer::timeout, this, [this] { pollCameraFrame(); });
    connect(captureButton_, &QPushButton::clicked, this, [this] { captureFrame(); });
    connect(confirmButton_, &QPushButton::clicked, this, [this] { startProcessing(); });
    connect(retakeButton_, &QPushButton::clicked, this, [this] { resetCapture(); });
}

void CaptureWindow::startCamera()
{
    releaseCamera();
    setState(State::Starting);
    setProgress(0, "正在初始化相机");

    LightFieldCapture::HoloInData config;
    config.iHoloExposeMode = 1;
    config.iHoloExposeVal = 15000;
    config.iHoloId = 0;
    config.dHoloFrameRate = 6.0;
    config.iHoloMissThreshold = 100;
    config.bIsReadTeamptureBySerial = false;
    config.strSerialPort = "";
    config.iSerialBaudRate = 9600;
    config.iSerialDataBits = 8;
    config.iSerialStopBits = 1;
    config.iSerialParity = 0;
    config.strParseCfgPath = QDir::toNativeSeparators(cameraConfigPath_).toStdString();
    std::replace(config.strParseCfgPath.begin(), config.strParseCfgPath.end(), '\\', '/');

    capture_ = std::make_unique<LightFieldCapture>();
    if (!capture_->initialize(&config)) {
        releaseCamera();
        hasLiveFrame_ = false;
        setState(State::Error);
        setProgress(0, "相机初始化失败");
        QMessageBox::warning(this, "相机初始化失败",
            "无法初始化相机，请检查相机连接和配置目录:\n" + cameraConfigPath_);
        return;
    }

    hasLiveFrame_ = false;
    setState(State::Live);
    setProgress(0, "等待相机画面");
    frameTimer_->start();
}

void CaptureWindow::releaseCamera()
{
    if (frameTimer_) {
        frameTimer_->stop();
    }
    capture_.reset();
}

void CaptureWindow::pollCameraFrame()
{
    if (!capture_ || state_ != State::Live) {
        return;
    }
    if (capture_->hasError()) {
        const QString errorMessage = QString::fromStdString(capture_->lastError());
        releaseCamera();
        setState(State::Error);
        setProgress(0, "相机采集失败");
        QMessageBox::warning(this, "相机采集失败",
            errorMessage.isEmpty() ? "相机采集线程已停止。" : errorMessage);
        return;
    }

    LightFieldCapture::HoloOutData frame;
    bool gotFrame = false;
    while (capture_->GetHoloOutData(frame)) {
        gotFrame = true;
    }
    if (!gotFrame || frame.img2d.empty() || frame.img3d.empty()) {
        return;
    }

    latestRgb_ = frame.img2d.clone();
    latestDepthForPipeline_ = frame.img3d.clone();
    latestDepthDisplay_ = frame.depthMap.empty() ? frame.img3d.clone() : frame.depthMap.clone();
    hasLiveFrame_ = true;
    setPreviewImages(latestRgb_, latestDepthDisplay_);

    captureButton_->setEnabled(true);
    if (progressBar_->value() == 0) {
        progressText_->setText("实时预览中");
    }
}

void CaptureWindow::captureFrame()
{
    if (!hasLiveFrame_ || latestRgb_.empty() || latestDepthForPipeline_.empty()) {
        return;
    }

    frozenRgb_ = latestRgb_.clone();
    frozenDepthForPipeline_ = latestDepthForPipeline_.clone();
    frozenDepthDisplay_ = latestDepthDisplay_.clone();
    if (frameTimer_) {
        frameTimer_->stop();
    }
    setPreviewImages(frozenRgb_, frozenDepthDisplay_);
    setState(State::Frozen);
    setProgress(0, "已定格当前帧");
}

void CaptureWindow::resetCapture()
{
    frozenRgb_.release();
    frozenDepthForPipeline_.release();
    frozenDepthDisplay_.release();
    setPreviewImages(cv::Mat(), cv::Mat());
    setProgress(0, "重新进入实时预览");

    if (!capture_) {
        startCamera();
        return;
    }

    setState(State::Live);
    if (frameTimer_) {
        frameTimer_->start();
    }
}

void CaptureWindow::startProcessing()
{
    if (frozenRgb_.empty() || frozenDepthForPipeline_.empty()) {
        return;
    }

    setState(State::Processing);
    confirmTimer_.restart();
    setProgress(1, "保存当前帧");
    releaseCamera();

    QString errorMessage;
    if (!preparePipelineInput(&errorMessage) || !writePipelineConfig(&errorMessage)) {
        setState(State::Frozen);
        QMessageBox::critical(this, "准备处理失败", errorMessage);
        return;
    }

    startPipelineThread();
}

bool CaptureWindow::preparePipelineInput(QString* errorMessage)
{
    if (!removeDirectoryIfExists(inputRoot())
        || !removeDirectoryIfExists(QDir(outputRoot()).filePath("multiview"))
        || !removeDirectoryIfExists(QDir(outputRoot()).filePath("elemental"))) {
        *errorMessage = "无法清理旧输出目录:\n" + outputRoot();
        return false;
    }
    if (!QDir().mkpath(inputRoot())) {
        *errorMessage = "无法创建输入目录:\n" + inputRoot();
        return false;
    }

    const QString rgbPath = QDir(inputRoot()).filePath("0.jpg");
    const QString depthPath = QDir(inputRoot()).filePath("0.tiff");

    cv::Mat depthToSave = frozenDepthForPipeline_;
    if (depthToSave.type() != CV_32FC3) {
        cv::Mat converted;
        depthToSave.convertTo(converted, CV_32FC3);
        depthToSave = converted;
    }

    const std::vector<int> tiffParams = { cv::IMWRITE_TIFF_COMPRESSION, 1 };
    if (!cv::imwrite(rgbPath.toStdString(), frozenRgb_)) {
        *errorMessage = "无法保存 RGB 图:\n" + rgbPath;
        return false;
    }
    if (!cv::imwrite(depthPath.toStdString(), depthToSave, tiffParams)) {
        *errorMessage = "无法保存深度 TIFF:\n" + depthPath;
        return false;
    }
    return true;
}

bool CaptureWindow::writePipelineConfig(QString* errorMessage)
{
    if (!QDir().mkpath(outputRoot())) {
        *errorMessage = "无法创建输出目录:\n" + outputRoot();
        return false;
    }

    const QString templatePath = QDir(projectRoot_).filePath("config/ui_pipeline_template.ini");
    QFile templateFile(templatePath);
    if (!templateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        *errorMessage = "无法读取 UI 处理配置模板:\n" + templatePath;
        return false;
    }

    QString configText = QString::fromUtf8(templateFile.readAll());
    configText.replace("{{depth_input_dir}}", forwardSlashes(inputRoot()));
    configText.replace("{{depth_config}}", forwardSlashes(QDir(projectRoot_).filePath("config/depth_to_pointcloud_config.cfg")));
    configText.replace("{{mesh_config}}", forwardSlashes(QDir(projectRoot_).filePath("config/mesh_config.cfg")));
    configText.replace("{{output_root}}", forwardSlashes(outputRoot()));
    configText.replace("{{log_file}}", forwardSlashes(pipelineLogPath()));

    QFile file(pipelineConfigPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        *errorMessage = "无法写入处理配置:\n" + pipelineConfigPath();
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << configText;
    return true;
}

void CaptureWindow::startPipelineThread()
{
    if (pipelineThread_) {
        return;
    }

    setProgress(2, "启动处理流程");
    pipelineExitCode_.store(1);
    pipelineNormalExit_.store(false);

    const QString appPath = QCoreApplication::applicationFilePath();
    const QString configPath = pipelineConfigPath();

    QThread* thread = QThread::create([this, appPath, configPath] {
        try {
            const QByteArray appArg = QFile::encodeName(appPath);
            const QByteArray configArg = QFile::encodeName(configPath);
            QByteArray configName("--config");
            QByteArray stageName("--stage");
            QByteArray stageArg("all");
            std::vector<char*> argv;
            argv.reserve(5);
            argv.push_back(const_cast<char*>(appArg.constData()));
            argv.push_back(configName.data());
            argv.push_back(const_cast<char*>(configArg.constData()));
            argv.push_back(stageName.data());
            argv.push_back(stageArg.data());

            const int exitCode = runHoloPipelineCli(static_cast<int>(argv.size()), argv.data());
            pipelineExitCode_.store(exitCode);
            pipelineNormalExit_.store(true);
        }
        catch (...) {
            pipelineExitCode_.store(1);
            pipelineNormalExit_.store(false);
        }
    });

    pipelineThread_ = thread;
    connect(thread, &QThread::started, this, [this] {
        setProgress(5, "处理中");
    });
    connect(thread, &QThread::finished, this, [this, thread] {
        const int exitCode = pipelineExitCode_.load();
        const bool normalExit = pipelineNormalExit_.load();
        if (pipelineThread_ == thread) {
            pipelineThread_ = nullptr;
        }
        thread->deleteLater();
        finishPipelineRun(exitCode, normalExit);
    });

    thread->start();
}

void CaptureWindow::finishPipelineRun(int exitCode, bool normalExit)
{
    setProgress(99, "整理输出结果");

    const double confirmSeconds = confirmTimer_.isValid()
        ? static_cast<double>(confirmTimer_.elapsed()) / 1000.0
        : 0.0;
    QFile logFile(pipelineLogPath());
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        QTextStream log(&logFile);
        log.setCodec("UTF-8");
        log << "\n[ui]\n";
        log << "confirm_to_finish_seconds=" << QString::number(confirmSeconds, 'f', 3) << "\n";
        log << "exit_code=" << exitCode << "\n";
        log << "exit_status=" << (normalExit ? "normal" : "crash") << "\n";
    }

    if (normalExit && exitCode == 0) {
        setState(State::Done);
        setProgress(100, "处理完成，结果已保存到 " + QDir::toNativeSeparators(outputRoot()));
        QMessageBox::information(this, "处理完成", "结果已保存到:\n" + QDir::toNativeSeparators(outputRoot()));
        return;
    }

    setState(State::Error);
    setProgress(progressBar_->value(), "处理失败");
    QMessageBox::critical(this, "处理失败",
        "处理流程没有成功结束，退出码: " + QString::number(exitCode));
}

void CaptureWindow::setState(State state)
{
    state_ = state;
    const bool processing = state == State::Processing;
    const bool live = state == State::Live;
    const bool frozen = state == State::Frozen;
    const bool doneOrError = state == State::Done || state == State::Error;

    captureButton_->setEnabled(live && hasLiveFrame_ && !processing);
    confirmButton_->setEnabled(frozen && !processing);
    retakeButton_->setEnabled((frozen || doneOrError) && !processing);

    if (state == State::Starting) {
        captureButton_->setEnabled(false);
        confirmButton_->setEnabled(false);
        retakeButton_->setEnabled(false);
    }
}

void CaptureWindow::setProgress(int value, const QString& text)
{
    progressBar_->setValue(std::max(0, std::min(100, value)));
    progressText_->setText(text);
}

void CaptureWindow::setPreviewImages(const cv::Mat& rgb, const cv::Mat& depthDisplay)
{
    rgbPreview_ = matToImage(rgb);
    depthPreview_ = depthToImage(depthDisplay);
    renderPreviews();
}

void CaptureWindow::renderPreviews()
{
    renderLabel(rgbLabel_, rgbPreview_, "显示rgb");
    renderLabel(depthLabel_, depthPreview_, "显示深度图");
}

void CaptureWindow::renderLabel(QLabel* label, const QImage& image, const QString& fallbackText)
{
    if (!label) {
        return;
    }
    if (image.isNull()) {
        label->setPixmap(QPixmap());
        label->setText(fallbackText);
        return;
    }

    label->setText(QString());
    label->setPixmap(QPixmap::fromImage(image).scaled(
        label->size().boundedTo(QSize(4096, 4096)),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation));
}

QString CaptureWindow::outputRoot() const
{
    return cleanPath(QDir(projectRoot_).filePath("output"));
}

QString CaptureWindow::inputRoot() const
{
    return cleanPath(QDir(outputRoot()).filePath("input"));
}

QString CaptureWindow::pipelineConfigPath() const
{
    return cleanPath(QDir(outputRoot()).filePath("holo_config.ui.ini"));
}

QString CaptureWindow::pipelineLogPath() const
{
    return cleanPath(QDir(outputRoot()).filePath("pipeline.log"));
}

void CaptureWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    renderPreviews();
}

void CaptureWindow::closeEvent(QCloseEvent* event)
{
    if (pipelineThread_ && pipelineThread_->isRunning()) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this,
            "处理中",
            "处理仍在进行，确定要退出吗？");
        if (answer != QMessageBox::Yes) {
            event->ignore();
            return;
        }
        setProgress(progressBar_->value(), "等待处理线程结束");
        pipelineThread_->requestInterruption();
        pipelineThread_->wait();
    }
    QMainWindow::closeEvent(event);
}

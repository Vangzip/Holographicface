#include "CaptureWindow.h"

#include "CaptureOrientation.h"
#include "DepthMeshModelMemory.h"
#include "HoloPipeline.h"
#include "Print9030Dialog.h"
#include "ProcessingSettingsDialog.h"
#include "elemental/ElementalMemoryResult.h"
#include "ui_CaptureWindow.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QPixmap>
#include <QSizePolicy>
#include <QStringList>
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

QString pathToQString(const std::filesystem::path& path)
{
#ifdef _WIN32
    return QString::fromStdWString(path.wstring());
#else
    return QString::fromStdString(path.string());
#endif
}

bool sameInputSelection(const PipelineInputSelection& left, const PipelineInputSelection& right)
{
    return left.mode == right.mode && left.directory == right.directory;
}

bool sameCameraSettings(const CameraCaptureSettings& left, const CameraCaptureSettings& right)
{
    return left.configDirectory == right.configDirectory
        && left.exposureMode == right.exposureMode
        && left.exposureValue == right.exposureValue
        && left.frameRate == right.frameRate
        && left.cameraInterface == right.cameraInterface
        && left.cameraType == right.cameraType
        && left.cameraId == right.cameraId
        && left.gpuId == right.gpuId
        && left.missedFrameThreshold == right.missedFrameThreshold
        && left.rotation == right.rotation;
}

} // namespace

CaptureWindow::CaptureWindow(const QString& projectRoot, const QString& cameraConfigPath, QWidget* parent)
    : QMainWindow(parent)
    , projectRoot_(QDir(projectRoot).absolutePath())
    , settingsPaths_(ProcessingSettingsPaths::fromProjectRoot(projectRoot_))
    , settings_(defaultProcessingSettings(projectRoot_, cameraConfigPath))
{
    buildUi();
    QString settingsError;
    if (!loadProcessingSettings(settingsPaths_, &settings_, &settingsError)) {
        QMessageBox::warning(this, QString::fromUtf8("读取设置失败"),
            QString::fromUtf8("将使用内置默认值。\n") + settingsError);
    }
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
    settingsButton_ = ui_->settingsButton;

    frameTimer_ = new QTimer(this);
    frameTimer_->setInterval(30);
    connect(frameTimer_, &QTimer::timeout, this, [this] { pollCameraFrame(); });
    connect(captureButton_, &QPushButton::clicked, this, [this] { captureFrame(); });
    connect(confirmButton_, &QPushButton::clicked, this, [this] { startProcessing(); });
    connect(retakeButton_, &QPushButton::clicked, this, [this] { resetCapture(); });
    connect(settingsButton_, &QPushButton::clicked, this, [this] { openProcessingSettings(); });
}

void CaptureWindow::startCamera()
{
    releaseCamera();
    if (settings_.input.isExternal()) {
        setState(State::Frozen);
        return;
    }
    setState(State::Starting);
    setProgress(0, "正在初始化相机");

    LightFieldCapture::HoloInData config = makeCameraInput(settings_.camera);

    capture_ = std::make_unique<LightFieldCapture>();
    if (!capture_->initialize(&config)) {
        releaseCamera();
        hasLiveFrame_ = false;
        setState(State::Error);
        setProgress(0, "相机初始化失败");
        QMessageBox::warning(this, "相机初始化失败",
            "无法初始化相机，请检查相机连接和配置目录:\n" + settings_.camera.configDirectory);
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

    const cv::Mat& depthDisplaySource =
        frame.depthMap.empty() ? frame.img3d : frame.depthMap;
    cv::Mat orientedRgb = rotateCaptureImage(frame.img2d, settings_.camera.rotation);
    cv::Mat orientedDepth = rotateCaptureSpatialDepth(frame.img3d, settings_.camera.rotation);
    cv::Mat orientedDepthDisplay = rotateCaptureImage(depthDisplaySource, settings_.camera.rotation);

    latestRgb_ = std::move(orientedRgb);
    latestDepthForPipeline_ = std::move(orientedDepth);
    latestDepthDisplay_ = std::move(orientedDepthDisplay);
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
    if (!settings_.input.isExternal()
        && (frozenRgb_.empty() || frozenDepthForPipeline_.empty())) {
        return;
    }

    PipelineInputFiles selectedFiles;
    QString errorMessage;
    if (!resolveSelectedInput(&selectedFiles, &errorMessage)) {
        QMessageBox::warning(this, QString::fromUtf8("输入设置无效"), errorMessage);
        return;
    }

    setState(State::Processing);
    {
        std::lock_guard<std::mutex> lock(elementalResultMutex_);
        elementalResult_.reset();
        resultSaveReport_.reset();
    }
    activeResultTimestamp_ = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    confirmTimer_.restart();
    setProgress(1, "保存当前帧");
    releaseCamera();

    const bool prepared = settings_.input.isExternal()
        || preparePipelineInput(&errorMessage);
    if (!prepared || !writePipelineConfig(&errorMessage)) {
        setState(State::Frozen);
        QMessageBox::critical(this, "准备处理失败", errorMessage);
        return;
    }

    startPipelineThread();
}

void CaptureWindow::openProcessingSettings()
{
    const ProcessingSettings previous = settings_;
    bool cameraReinitialized = false;
    ProcessingSettingsDialog dialog(this);
    dialog.setSettings(settings_);
    dialog.setBusy(state_ == State::Processing || state_ == State::Starting);
    connect(&dialog, &ProcessingSettingsDialog::printRequested,
        this, [this, &dialog] {
            dialog.hide();
            openPrintSettings();
            dialog.show();
        });
    connect(&dialog, &ProcessingSettingsDialog::cameraTestRequested,
        this, [this, &dialog](const CameraCaptureSettings& camera) {
            QString message;
            const bool success = testCameraConnection(camera, &message);
            dialog.setCameraTestResult(success, message);
        });
    connect(&dialog, &ProcessingSettingsDialog::cameraReinitializeRequested,
        this, [this, &cameraReinitialized](const CameraCaptureSettings& camera) {
            settings_.camera = camera;
            cameraReinitialized = true;
            startCamera();
        });
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const ProcessingSettings candidate = dialog.settings();
    QString errorMessage;
    if (!saveProcessingSettings(settingsPaths_, candidate, &errorMessage)) {
        QMessageBox::warning(this, QString::fromUtf8("保存设置失败"), errorMessage);
        return;
    }
    settings_ = candidate;

    if (!sameInputSelection(previous.input, settings_.input)) {
        if (settings_.input.isExternal()) {
            if (!applySelectedInput(&errorMessage)) {
                settings_.input.clear();
                QMessageBox::warning(this, QString::fromUtf8("输入设置无效"), errorMessage);
                startCamera();
            }
        }
        else {
            clearInputSettingsAndResumeCamera();
        }
    }
    else if (!cameraReinitialized && !settings_.input.isExternal()
        && !sameCameraSettings(previous.camera, settings_.camera)) {
        startCamera();
    }
}

bool CaptureWindow::testCameraConnection(
    const CameraCaptureSettings& camera,
    QString* message)
{
    const State previousState = state_;
    releaseCamera();

    LightFieldCapture testCapture;
    LightFieldCapture::HoloInData config = makeCameraInput(camera);
    bool success = testCapture.initialize(&config);
    QString detail;
    if (!success) {
        detail = QString::fromUtf8("相机初始化失败，请检查连接、接口和标定方案");
    }
    else {
        QElapsedTimer timeout;
        timeout.start();
        while (timeout.elapsed() < 5000) {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            if (testCapture.hasError()) {
                success = false;
                detail = QString::fromStdString(testCapture.lastError());
                break;
            }
            LightFieldCapture::HoloOutData frame;
            bool receivedFrame = false;
            while (testCapture.GetHoloOutData(frame)) {
                receivedFrame = true;
            }
            if (receivedFrame && !frame.img2d.empty() && !frame.img3d.empty()) {
                success = true;
                detail = QString::fromUtf8("相机连接正常，已收到有效画面");
                break;
            }
            success = false;
            QThread::msleep(10);
        }
        if (!success && detail.isEmpty()) {
            detail = QString::fromUtf8("相机已初始化，但 5 秒内未收到有效画面");
        }
        testCapture.release();
    }

    if (previousState == State::Live || previousState == State::Frozen) {
        startCamera();
        if (previousState == State::Frozen && capture_) {
            frameTimer_->stop();
            setPreviewImages(frozenRgb_, frozenDepthDisplay_);
            setState(State::Frozen);
        }
    }
    else {
        setState(previousState);
    }
    if (message) {
        *message = detail;
    }
    return success;
}

void CaptureWindow::openPrintSettings()
{
    std::shared_ptr<const ElementalMemoryResult> result;
    {
        std::lock_guard<std::mutex> lock(elementalResultMutex_);
        result = elementalResult_;
    }
    Print9030Dialog dialog(projectRoot_, this);
    dialog.setElementalMemoryResult(std::move(result));
    dialog.exec();
}

void CaptureWindow::showResultSaveWarnings()
{
    std::shared_ptr<ResultSaveReport> report;
    {
        std::lock_guard<std::mutex> lock(elementalResultMutex_);
        report = std::move(resultSaveReport_);
    }
    if (!report || !report->hasWarnings()) {
        return;
    }

    QStringList warningDetails;
    for (const ResultSaveWarning& warning : report->warnings()) {
        const QString outputDirectory = QDir::toNativeSeparators(
            QString::fromStdWString(warning.outputDirectory.wstring()));
        warningDetails.append(
            QString::fromUtf8("%1\n目录：%2\n原因：%3")
                .arg(QString::fromStdString(warning.resultType))
                .arg(outputDirectory)
                .arg(QString::fromLocal8Bit(warning.message.c_str())));
    }

    QMessageBox::warning(
        this,
        QString::fromUtf8("部分结果保存失败"),
        QString::fromUtf8("打印数据仍可正常使用。以下可选结果未能完整保存：\n\n")
            + warningDetails.join("\n\n"));
}

bool CaptureWindow::preparePipelineInput(QString* errorMessage)
{
    if (!removeDirectoryIfExists(inputRoot())) {
        *errorMessage = "无法清理旧输入目录:\n" + inputRoot();
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

bool CaptureWindow::resolveSelectedInput(
    PipelineInputFiles* files,
    QString* errorMessage) const
{
    if (settings_.input.mode == PipelineInputMode::Multiview) {
        // The dialog already validated the full grid. The worker validates it
        // again, off the UI thread, immediately before elemental processing.
        if (files) {
            *files = PipelineInputFiles{};
            files->multiviewDirectory = settings_.input.directory;
        }
        return true;
    }

    std::string validationError;
    if (!resolvePipelineInput(
            settings_.input, MultiviewInputSpec{}, files, &validationError)) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8(validationError.c_str());
        }
        return false;
    }
    return true;
}

bool CaptureWindow::applySelectedInput(QString* errorMessage)
{
    PipelineInputFiles files;
    if (!resolveSelectedInput(&files, errorMessage)) {
        return false;
    }
    if (settings_.input.mode == PipelineInputMode::Mesh) {
        MeshMemoryResult meshInput;
        std::string validationError;
        if (!loadPipelineMeshInput(files, &meshInput, &validationError)) {
            if (errorMessage) {
                *errorMessage = QString::fromUtf8(validationError.c_str());
            }
            return false;
        }
    }

    releaseCamera();
    hasLiveFrame_ = false;
    latestRgb_.release();
    latestDepthForPipeline_.release();
    latestDepthDisplay_.release();
    frozenRgb_.release();
    frozenDepthForPipeline_.release();
    frozenDepthDisplay_.release();

    if (settings_.input.mode == PipelineInputMode::RgbDepth) {
        frozenRgb_ = cv::imread(files.rgbPath.string(), cv::IMREAD_COLOR);
        frozenDepthForPipeline_ = cv::imread(files.depthPath.string(), cv::IMREAD_UNCHANGED);
        if (frozenRgb_.empty() || frozenDepthForPipeline_.empty()) {
            if (errorMessage) {
                *errorMessage = QString::fromUtf8("无法读取所选 RGB 或深度图文件。");
            }
            return false;
        }
        frozenDepthDisplay_ = frozenDepthForPipeline_.clone();
        setPreviewImages(frozenRgb_, frozenDepthDisplay_);
        setProgress(0, QString::fromUtf8("已选择 RGB、深度图输入，点击确认开始处理"));
    }
    else {
        setPreviewImages(cv::Mat(), cv::Mat());
        const QString sourceName = settings_.input.mode == PipelineInputMode::Mesh
            ? QStringLiteral("mesh") : QStringLiteral("multiview");
        setProgress(0, QString::fromUtf8("已选择 %1 输入，点击确认开始处理").arg(sourceName));
    }

    setState(State::Frozen);
    return true;
}

void CaptureWindow::clearInputSettingsAndResumeCamera()
{
    settings_.input.clear();
    frozenRgb_.release();
    frozenDepthForPipeline_.release();
    frozenDepthDisplay_.release();
    setPreviewImages(cv::Mat(), cv::Mat());
    startCamera();
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
    const QString selectedDirectory = settings_.input.isExternal()
        ? cleanPath(pathToQString(settings_.input.directory)) : QString();
    const QString depthInputDirectory = settings_.input.mode == PipelineInputMode::RgbDepth
        ? selectedDirectory : inputRoot();
    configText.replace("{{input_mode}}",
        QString::fromStdString(pipelineInputModeName(settings_.input.mode)));
    configText.replace("{{input_dir}}", forwardSlashes(selectedDirectory));
    configText.replace("{{depth_input_dir}}", forwardSlashes(depthInputDirectory));
    configText.replace("{{depth_config}}", forwardSlashes(QDir(projectRoot_).filePath("config/depth_to_pointcloud_config.cfg")));
    configText.replace("{{mesh_config}}", forwardSlashes(QDir(projectRoot_).filePath("config/mesh_config.cfg")));
    configText.replace("{{output_root}}", forwardSlashes(outputRoot()));
    configText.replace("{{log_file}}", forwardSlashes(pipelineLogPath()));
    configText.replace("{{save_mesh_result}}", settings_.saveResults.mesh ? "true" : "false");
    configText.replace("{{save_multiview_result}}", settings_.saveResults.multiview ? "true" : "false");
    configText.replace("{{save_elemental_result}}", settings_.saveResults.elemental ? "true" : "false");
    configText.replace("{{result_timestamp}}", activeResultTimestamp_);
    const PipelineUiSettings& pipeline = settings_.pipeline;
    configText.replace("{{multiview_camera_distance_scale}}",
        QString::number(distanceScaleFromSubjectSize(pipeline.subjectSize), 'g', 15));
    configText.replace("{{multiview_camera_center_offset_x}}", QString::number(pipeline.centerX, 'g', 15));
    configText.replace("{{multiview_camera_center_offset_y}}", QString::number(pipeline.centerY, 'g', 15));
    configText.replace("{{multiview_camera_center_offset_z}}", QString::number(pipeline.centerZ, 'g', 15));
    configText.replace("{{multiview_initial_rotate_x_deg}}", QString::number(pipeline.rotateXDeg, 'g', 15));
    configText.replace("{{multiview_initial_rotate_z_deg}}", QString::number(pipeline.rotateZDeg, 'g', 15));
    configText.replace("{{multiview_capture_flip_vertical}}", pipeline.captureFlipVertical ? "true" : "false");
    configText.replace("{{multiview_angle}}", QString::number(pipeline.multiviewAngle));
    configText.replace("{{multiview_per}}", QString::number(pipeline.multiviewPer));
    configText.replace("{{multiview_resolution}}", QString::number(pipeline.multiviewResolution));
    configText.replace("{{multiview_atlas_size}}", QString::number(pipeline.atlasSize));
    configText.replace("{{target_rows}}", QString::number(pipeline.targetRows));
    configText.replace("{{target_cols}}", QString::number(pipeline.targetCols));
    configText.replace("{{jpg_quality}}", QString::number(pipeline.jpgQuality));
    configText.replace("{{elemental_writer_threads}}", QString::number(pipeline.writerThreads));
    configText.replace("{{elemental_flip_source_y}}", pipeline.elementalFlipSourceY ? "true" : "false");
    configText.replace("{{elemental_flip_view_rows}}", pipeline.elementalFlipViewRows ? "true" : "false");

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
            auto elementalResult = std::make_shared<ElementalMemoryResult>();
            auto saveReport = std::make_shared<ResultSaveReport>();
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

            const int exitCode = runHoloPipelineCliWithResult(
                static_cast<int>(argv.size()), argv.data(), elementalResult.get(), saveReport.get());
            if (exitCode == 0) {
                std::lock_guard<std::mutex> lock(elementalResultMutex_);
                elementalResult_ = std::move(elementalResult);
                resultSaveReport_ = std::move(saveReport);
            }
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
        setProgress(100, "处理完成，打印数据已就绪");
        openPrintSettings();
        showResultSaveWarnings();
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
    const bool externalInput = settings_.input.isExternal();

    captureButton_->setEnabled(!externalInput && live && hasLiveFrame_ && !processing);
    confirmButton_->setEnabled(frozen && !processing);
    retakeButton_->setEnabled(!externalInput && (frozen || doneOrError) && !processing);
    settingsButton_->setEnabled(!processing && state != State::Starting);

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
    return cleanPath(settings_.pipeline.outputRoot);
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

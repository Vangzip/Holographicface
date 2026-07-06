#include "CaptureWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QPixmap>
#include <QRegularExpression>
#include <QSizePolicy>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

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

QLabel* makePreviewLabel(const QString& text)
{
    QLabel* label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumSize(240, 220);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    label->setFrameShape(QFrame::Box);
    label->setLineWidth(2);
    label->setStyleSheet("QLabel { background: #ffffff; color: #111111; border: 2px solid #111111; }");
    return label;
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
    if (pipelineProcess_) {
        pipelineProcess_->kill();
        pipelineProcess_->waitForFinished(3000);
    }
    releaseCamera();
}

void CaptureWindow::buildUi()
{
    setWindowTitle("MergeHolo 拍照处理");
    resize(860, 560);

    QWidget* central = new QWidget(this);
    QVBoxLayout* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(46, 38, 46, 38);
    rootLayout->setSpacing(14);

    QHBoxLayout* previewLayout = new QHBoxLayout;
    previewLayout->setSpacing(0);
    rgbLabel_ = makePreviewLabel("显示rgb");
    depthLabel_ = makePreviewLabel("显示深度图");
    previewLayout->addStretch(1);
    previewLayout->addWidget(rgbLabel_, 1);
    previewLayout->addWidget(depthLabel_, 1);
    previewLayout->addStretch(1);
    rootLayout->addLayout(previewLayout, 1);

    QHBoxLayout* progressLayout = new QHBoxLayout;
    QLabel* progressCaption = new QLabel("当前进度:");
    progressCaption->setFixedWidth(78);
    progressText_ = new QLabel("初始化相机");
    progressText_->setAlignment(Qt::AlignCenter);
    progressBar_ = new QProgressBar;
    progressBar_->setRange(0, 100);
    progressBar_->setTextVisible(false);
    progressBar_->setFixedHeight(20);
    progressBar_->setStyleSheet(
        "QProgressBar { border: 2px solid #111111; background: #ffffff; }"
        "QProgressBar::chunk { background: #2f80ed; }");
    progressLayout->addWidget(progressCaption);
    progressLayout->addWidget(progressBar_, 1);
    rootLayout->addLayout(progressLayout);
    rootLayout->addWidget(progressText_);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(84);
    captureButton_ = new QPushButton("拍照按钮");
    confirmButton_ = new QPushButton("确认");
    retakeButton_ = new QPushButton("重新拍照");
    const QList<QPushButton*> buttons = { captureButton_, confirmButton_, retakeButton_ };
    for (QPushButton* button : buttons) {
        button->setMinimumSize(138, 56);
        button->setStyleSheet(
            "QPushButton { background: #ffffff; border: 2px solid #111111; font-size: 16px; }"
            "QPushButton:disabled { color: #888888; border-color: #888888; background: #f2f2f2; }");
    }
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(captureButton_);
    buttonLayout->addWidget(confirmButton_);
    buttonLayout->addWidget(retakeButton_);
    buttonLayout->addStretch(1);
    rootLayout->addLayout(buttonLayout);

    setCentralWidget(central);

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
    setProgress(1, "保存当前帧");
    releaseCamera();

    QString errorMessage;
    if (!preparePipelineInput(&errorMessage) || !writePipelineConfig(&errorMessage)) {
        setState(State::Frozen);
        QMessageBox::critical(this, "准备处理失败", errorMessage);
        return;
    }

    startPipelineProcess();
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

    QFile file(pipelineConfigPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        *errorMessage = "无法写入处理配置:\n" + pipelineConfigPath();
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << "depth_input_dir=" << forwardSlashes(inputRoot()) << "\n";
    out << "depth_config=" << forwardSlashes(QDir(projectRoot_).filePath("config/depth_to_pointcloud_config.cfg")) << "\n";
    out << "mesh_config=" << forwardSlashes(QDir(projectRoot_).filePath("config/mesh_config.cfg")) << "\n";
    out << "mesh_obj=\n";
    out << "output_root=" << forwardSlashes(outputRoot()) << "\n";
    out << "multiview_out_dir=multiview\n";
    out << "elemental_out_dir=elemental\n";
    out << "model_type=obj\n";
    out << "multiview_camera_distance_scale=2.0\n";
    out << "multiview_camera_center_offset_x=0.0\n";
    out << "multiview_camera_center_offset_y=0.0\n";
    out << "multiview_camera_center_offset_z=0.0\n";
    out << "multiview_camera_eye_dir_x=0.0\n";
    out << "multiview_camera_eye_dir_y=-1.0\n";
    out << "multiview_camera_eye_dir_z=0.0\n";
    out << "multiview_camera_up_x=0.0\n";
    out << "multiview_camera_up_y=0.0\n";
    out << "multiview_camera_up_z=1.0\n";
    out << "multiview_camera_fovy_deg=0.0\n";
    out << "multiview_camera_z_near=0.0\n";
    out << "multiview_camera_z_far=0.0\n";
    out << "multiview_capture_flip_vertical=true\n";
    out << "multiview_angle=90\n";
    out << "multiview_per=3\n";
    out << "multiview_resolution=150\n";
    out << "view_name_digits=3\n";
    out << "target_rows=150\n";
    out << "target_cols=150\n";
    out << "jpg_quality=100\n";
    out << "elemental_writer_threads=1\n";
    out << "elemental_flip_source_y=true\n";
    out << "elemental_flip_view_rows=true\n";
    out << "run_depth_pointcloud=true\n";
    out << "run_mesh=true\n";
    out << "run_textured_model=true\n";
    out << "run_multiview=true\n";
    out << "run_elemental=true\n";
    return true;
}

void CaptureWindow::startPipelineProcess()
{
    setProgress(2, "启动处理流程");
    stdoutBuffer_.clear();
    stderrBuffer_.clear();

    pipelineProcess_ = new QProcess(this);
    pipelineProcess_->setWorkingDirectory(projectRoot_);
    pipelineProcess_->setProgram(QCoreApplication::applicationFilePath());
    pipelineProcess_->setArguments({ "--pipeline", "--config", pipelineConfigPath(), "--stage", "all" });

    connect(pipelineProcess_, &QProcess::readyReadStandardOutput, this, [this] {
        consumeProcessOutput(stdoutBuffer_, pipelineProcess_->readAllStandardOutput());
    });
    connect(pipelineProcess_, &QProcess::readyReadStandardError, this, [this] {
        consumeProcessOutput(stderrBuffer_, pipelineProcess_->readAllStandardError());
    });
    connect(pipelineProcess_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
            finishPipelineProcess(exitCode, exitStatus);
        });
    connect(pipelineProcess_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (pipelineProcess_) {
            setProgress(progressBar_->value(), "处理进程启动失败");
        }
    });

    pipelineProcess_->start();
}

void CaptureWindow::consumeProcessOutput(QByteArray& buffer, const QByteArray& chunk)
{
    buffer.append(chunk);
    int newlineIndex = -1;
    while ((newlineIndex = buffer.indexOf('\n')) >= 0) {
        const QByteArray rawLine = buffer.left(newlineIndex);
        buffer.remove(0, newlineIndex + 1);
        handleProcessLine(QString::fromLocal8Bit(rawLine).trimmed());
    }
}

void CaptureWindow::handleProcessLine(const QString& line)
{
    if (line.isEmpty()) {
        return;
    }

    if (line.startsWith("[depth]")) {
        setProgress(5, "深度图转点云");
    }
    else if (line.contains("[timing] stage depth")) {
        setProgress(20, "点云生成完成");
    }
    else if (line.startsWith("[mesh]")) {
        setProgress(25, "点云重建网格");
    }
    else if (line.contains("[timing] stage mesh")) {
        setProgress(45, "网格生成完成");
    }
    else if (line.startsWith("[model]")) {
        setProgress(50, "生成贴图模型");
    }
    else if (line.contains("[timing] stage model")) {
        setProgress(65, "贴图模型完成");
    }
    else if (line.startsWith("m_height:") || line.contains("init eye")) {
        setProgress(std::max(progressBar_->value(), 70), "生成多视角图");
    }
    else if (line.contains("[timing] stage multiview")) {
        setProgress(82, "多视角图完成");
    }
    else if (line.startsWith("[elemental] loaded view")) {
        setProgress(std::max(progressBar_->value(), 86), "加载多视角缓存");
    }
    else if (line.startsWith("[elemental] wrote ")) {
        static const QRegularExpression re("\\[elemental\\] wrote\\s+(\\d+)/(\\d+)\\s+images");
        const QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            const int done = match.captured(1).toInt();
            const int total = std::max(1, match.captured(2).toInt());
            const int value = 86 + static_cast<int>((static_cast<double>(done) / total) * 13.0);
            setProgress(std::min(99, value), "生成结果图 " + match.captured(1) + "/" + match.captured(2));
        }
    }
    else if (line.contains("[timing] stage elemental")) {
        setProgress(99, "整理输出结果");
    }
}

void CaptureWindow::finishPipelineProcess(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!stdoutBuffer_.isEmpty()) {
        handleProcessLine(QString::fromLocal8Bit(stdoutBuffer_).trimmed());
        stdoutBuffer_.clear();
    }
    if (!stderrBuffer_.isEmpty()) {
        handleProcessLine(QString::fromLocal8Bit(stderrBuffer_).trimmed());
        stderrBuffer_.clear();
    }

    pipelineProcess_->deleteLater();
    pipelineProcess_ = nullptr;

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
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

void CaptureWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    renderPreviews();
}

void CaptureWindow::closeEvent(QCloseEvent* event)
{
    if (pipelineProcess_) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this,
            "处理中",
            "处理仍在进行，确定要退出吗？");
        if (answer != QMessageBox::Yes) {
            event->ignore();
            return;
        }
        pipelineProcess_->kill();
        pipelineProcess_->waitForFinished(3000);
    }
    QMainWindow::closeEvent(event);
}

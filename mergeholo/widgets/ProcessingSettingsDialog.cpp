#include "ProcessingSettingsDialog.h"

#include "ui_ProcessingSettingsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

QLineEdit* pathEdit(QWidget* parent, const char* name)
{
    auto* edit = new QLineEdit(parent);
    edit->setObjectName(name);
    edit->setReadOnly(true);
    return edit;
}

QPushButton* browseButton(QWidget* parent, const char* name)
{
    auto* button = new QPushButton(QString::fromUtf8("浏览..."), parent);
    button->setObjectName(name);
    button->setMinimumWidth(88);
    return button;
}

QSpinBox* integerSpin(QWidget* parent, const char* name, int minimum, int maximum)
{
    auto* spin = new QSpinBox(parent);
    spin->setObjectName(name);
    spin->setRange(minimum, maximum);
    return spin;
}

QDoubleSpinBox* decimalSpin(
    QWidget* parent,
    const char* name,
    double minimum,
    double maximum,
    int decimals,
    double step)
{
    auto* spin = new QDoubleSpinBox(parent);
    spin->setObjectName(name);
    spin->setRange(minimum, maximum);
    spin->setDecimals(decimals);
    spin->setSingleStep(step);
    return spin;
}

QString displayPath(const std::filesystem::path& path)
{
#ifdef _WIN32
    return QDir::toNativeSeparators(QString::fromStdWString(path.wstring()));
#else
    return QDir::toNativeSeparators(QString::fromStdString(path.string()));
#endif
}

std::filesystem::path filesystemPath(const QString& path)
{
#ifdef _WIN32
    return std::filesystem::path(path.toStdWString()).lexically_normal();
#else
    return std::filesystem::path(path.toStdString()).lexically_normal();
#endif
}

} // namespace

ProcessingSettingsDialog::ProcessingSettingsDialog(QWidget* parent)
    : QDialog(parent)
    , ui_(new Ui::ProcessingSettingsDialog)
{
    ui_->setupUi(this);
    setWindowModality(Qt::ApplicationModal);
    buildCommonPage();
    buildImagingPage();
    buildAdvancedPage();
    buildDevicePage();
    buildPlaceholderPages();
    ui_->settingsNavigation->setCurrentRow(0);

    connect(ui_->settingsNavigation, &QListWidget::currentRowChanged,
        this, [this](int row) {
            if (row == 4) {
                emit printRequested();
                const QSignalBlocker blocker(ui_->settingsNavigation);
                ui_->settingsNavigation->setCurrentRow(selectedPage_);
                return;
            }
            if (row >= 0 && row < ui_->settingsPages->count()) {
                selectedPage_ = row;
                ui_->settingsPages->setCurrentIndex(row);
            }
        });
    connect(ui_->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui_->applyButton, &QPushButton::clicked, this, &ProcessingSettingsDialog::accept);
    connect(ui_->restoreDefaultsButton, &QPushButton::clicked,
        this, &ProcessingSettingsDialog::restoreCurrentPageDefaults);
}

ProcessingSettingsDialog::~ProcessingSettingsDialog() = default;

void ProcessingSettingsDialog::setSettings(const ProcessingSettings& settings)
{
    draft_ = settings;
    populateCommonPage();
    populateImagingPage();
    populateAdvancedPage();
    populateDevicePage();
}

ProcessingSettings ProcessingSettingsDialog::settings() const
{
    ProcessingSettings result = draft_;
    collectCommonPage(&result);
    collectImagingPage(&result);
    collectAdvancedPage(&result);
    collectDevicePage(&result);
    return result;
}

void ProcessingSettingsDialog::setBusy(bool busy)
{
    busy_ = busy;
    ui_->devicePage->setEnabled(!busy);
    ui_->applyButton->setEnabled(!busy);
}

void ProcessingSettingsDialog::setCameraTestResult(bool success, const QString& message)
{
    QLabel* status = findChild<QLabel*>("cameraStatusLabel");
    if (!status) {
        return;
    }
    status->setText(message.isEmpty()
        ? (success ? QString::fromUtf8("相机连接测试成功")
                   : QString::fromUtf8("相机连接测试失败"))
        : message);
}

void ProcessingSettingsDialog::selectPage(int pageIndex)
{
    if (pageIndex < 0 || pageIndex >= ui_->settingsPages->count()) {
        return;
    }
    ui_->settingsNavigation->setCurrentRow(pageIndex);
}

void ProcessingSettingsDialog::accept()
{
    const ProcessingSettings candidate = settings();
    const QString error = validateProcessingSettings(candidate);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("设置无效"), error);
        return;
    }
    draft_ = candidate;
    QDialog::accept();
}

void ProcessingSettingsDialog::buildCommonPage()
{
    auto* inputGroup = new QGroupBox(QString::fromUtf8("输入与输出"), ui_->commonPage);
    auto* inputLayout = new QFormLayout(inputGroup);
    auto* inputMode = new QComboBox(inputGroup);
    inputMode->setObjectName("inputModeCombo");
    inputMode->addItem(QString::fromUtf8("相机采集"), static_cast<int>(PipelineInputMode::Camera));
    inputMode->addItem(QString::fromUtf8("RGB + 深度"), static_cast<int>(PipelineInputMode::RgbDepth));
    inputMode->addItem(QString::fromUtf8("网格模型"), static_cast<int>(PipelineInputMode::Mesh));
    inputMode->addItem(QString::fromUtf8("多视图图像"), static_cast<int>(PipelineInputMode::Multiview));
    inputLayout->addRow(QString::fromUtf8("输入来源"), inputMode);

    auto* inputRow = new QWidget(inputGroup);
    auto* inputRowLayout = new QHBoxLayout(inputRow);
    inputRowLayout->setContentsMargins(0, 0, 0, 0);
    inputRowLayout->addWidget(pathEdit(inputRow, "inputDirectoryEdit"), 1);
    inputRowLayout->addWidget(browseButton(inputRow, "inputBrowseButton"));
    inputLayout->addRow(QString::fromUtf8("输入目录"), inputRow);

    auto* outputRow = new QWidget(inputGroup);
    auto* outputRowLayout = new QHBoxLayout(outputRow);
    outputRowLayout->setContentsMargins(0, 0, 0, 0);
    outputRowLayout->addWidget(pathEdit(outputRow, "outputDirectoryEdit"), 1);
    outputRowLayout->addWidget(browseButton(outputRow, "outputBrowseButton"));
    inputLayout->addRow(QString::fromUtf8("输出目录"), outputRow);
    ui_->commonPageLayout->addWidget(inputGroup);

    auto* outputGroup = new QGroupBox(QString::fromUtf8("输出规格"), ui_->commonPage);
    auto* outputLayout = new QFormLayout(outputGroup);
    auto* angle = integerSpin(outputGroup, "multiviewAngleSpin", 1, 360);
    angle->setSuffix(QString::fromUtf8(" °"));
    outputLayout->addRow(QString::fromUtf8("视角范围"), angle);
    outputLayout->addRow(QString::fromUtf8("每度采样数"),
        integerSpin(outputGroup, "multiviewPerSpin", 1, 10));
    auto* resolution = integerSpin(outputGroup, "multiviewResolutionSpin", 16, 4096);
    resolution->setSuffix(QString::fromUtf8(" × 同宽"));
    outputLayout->addRow(QString::fromUtf8("单视图分辨率"), resolution);

    auto* elementalRow = new QWidget(outputGroup);
    auto* elementalLayout = new QHBoxLayout(elementalRow);
    elementalLayout->setContentsMargins(0, 0, 0, 0);
    elementalLayout->addWidget(integerSpin(elementalRow, "targetRowsSpin", 1, 4096));
    elementalLayout->addWidget(new QLabel(QString::fromUtf8("×"), elementalRow));
    elementalLayout->addWidget(integerSpin(elementalRow, "targetColsSpin", 1, 4096));
    elementalLayout->addStretch();
    outputLayout->addRow(QString::fromUtf8("Elemental 行列"), elementalRow);
    auto* summary = new QLabel(outputGroup);
    summary->setObjectName("derivedSummaryLabel");
    summary->setFrameShape(QFrame::StyledPanel);
    summary->setAlignment(Qt::AlignCenter);
    summary->setMinimumHeight(32);
    outputLayout->addRow(summary);
    ui_->commonPageLayout->addWidget(outputGroup);

    auto* saveGroup = new QGroupBox(QString::fromUtf8("保存内容"), ui_->commonPage);
    auto* saveLayout = new QVBoxLayout(saveGroup);
    auto* saveMesh = new QCheckBox(QString::fromUtf8("网格模型"), saveGroup);
    saveMesh->setObjectName("saveMeshCheck");
    auto* saveMultiview = new QCheckBox(QString::fromUtf8("多视图图像"), saveGroup);
    saveMultiview->setObjectName("saveMultiviewCheck");
    auto* saveElemental = new QCheckBox(QString::fromUtf8("Elemental 图像"), saveGroup);
    saveElemental->setObjectName("saveElementalCheck");
    saveLayout->addWidget(saveMesh);
    saveLayout->addWidget(saveMultiview);
    saveLayout->addWidget(saveElemental);
    ui_->commonPageLayout->addWidget(saveGroup);
    ui_->commonPageLayout->addStretch();

    connect(inputMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &ProcessingSettingsDialog::updateInputState);
    connect(findChild<QPushButton*>("inputBrowseButton"), &QPushButton::clicked,
        this, &ProcessingSettingsDialog::browseInputDirectory);
    connect(findChild<QPushButton*>("outputBrowseButton"), &QPushButton::clicked,
        this, &ProcessingSettingsDialog::browseOutputDirectory);
    for (const char* name : { "multiviewAngleSpin", "multiviewPerSpin",
             "targetRowsSpin", "targetColsSpin" }) {
        connect(findChild<QSpinBox*>(name), QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ProcessingSettingsDialog::updateDerivedSummary);
    }
}

void ProcessingSettingsDialog::buildPlaceholderPages()
{
}

void ProcessingSettingsDialog::buildImagingPage()
{
    auto* framingGroup = new QGroupBox(QString::fromUtf8("人物构图"), ui_->imagingPage);
    auto* framingLayout = new QFormLayout(framingGroup);
    framingLayout->addRow(QString::fromUtf8("人物大小"),
        decimalSpin(framingGroup, "subjectSizeSpin", 0.5, 5.0, 2, 0.1));
    framingLayout->addRow(QString::fromUtf8("左右位置"),
        decimalSpin(framingGroup, "centerXSpin", -1.0, 1.0, 2, 0.05));
    framingLayout->addRow(QString::fromUtf8("上下位置"),
        decimalSpin(framingGroup, "centerYSpin", -1.0, 1.0, 2, 0.05));
    framingLayout->addRow(QString::fromUtf8("远近位置"),
        decimalSpin(framingGroup, "centerZSpin", -1.0, 1.0, 2, 0.05));
    auto* centerButton = new QPushButton(QString::fromUtf8("居中"), framingGroup);
    centerButton->setObjectName("centerSubjectButton");
    framingLayout->addRow(QString(), centerButton);
    ui_->imagingPageLayout->addWidget(framingGroup);

    auto* poseGroup = new QGroupBox(QString::fromUtf8("姿态校正"), ui_->imagingPage);
    auto* poseLayout = new QFormLayout(poseGroup);
    auto* horizontal = decimalSpin(poseGroup, "rotateZSpin", -180.0, 180.0, 1, 1.0);
    horizontal->setSuffix(QString::fromUtf8(" °"));
    poseLayout->addRow(QString::fromUtf8("水平旋转"), horizontal);
    auto* pitch = decimalSpin(poseGroup, "rotateXSpin", -180.0, 180.0, 1, 1.0);
    pitch->setSuffix(QString::fromUtf8(" °"));
    poseLayout->addRow(QString::fromUtf8("俯仰旋转"), pitch);
    auto* resetPose = new QPushButton(QString::fromUtf8("重置姿态"), poseGroup);
    resetPose->setObjectName("resetPoseButton");
    poseLayout->addRow(QString(), resetPose);
    ui_->imagingPageLayout->addWidget(poseGroup);

    auto* qualityGroup = new QGroupBox(QString::fromUtf8("图像质量与方向"), ui_->imagingPage);
    auto* qualityLayout = new QFormLayout(qualityGroup);
    auto* quality = integerSpin(qualityGroup, "jpgQualitySpin", 1, 100);
    quality->setSuffix(QString::fromUtf8(" %"));
    qualityLayout->addRow(QString::fromUtf8("JPEG 质量"), quality);
    auto* captureFlip = new QCheckBox(QString::fromUtf8("垂直翻转采集图像"), qualityGroup);
    captureFlip->setObjectName("captureFlipCheck");
    qualityLayout->addRow(QString(), captureFlip);
    auto* direction = new QComboBox(qualityGroup);
    direction->setObjectName("elementalDirectionCombo");
    direction->addItems({ QString::fromUtf8("标准方向"),
        QString::fromUtf8("源图垂直翻转"), QString::fromUtf8("视点行翻转"),
        QString::fromUtf8("双向翻转") });
    qualityLayout->addRow(QString::fromUtf8("Elemental 输出方向"), direction);
    ui_->imagingPageLayout->addWidget(qualityGroup);

    auto* hint = new QLabel(
        QString::fromUtf8("这些参数直接影响人物在多视图图像中的大小、位置和方向"),
        ui_->imagingPage);
    hint->setFrameShape(QFrame::StyledPanel);
    hint->setAlignment(Qt::AlignCenter);
    hint->setMinimumHeight(34);
    ui_->imagingPageLayout->addWidget(hint);
    ui_->imagingPageLayout->addStretch();

    connect(centerButton, &QPushButton::clicked, this, [this] {
        findChild<QDoubleSpinBox*>("centerXSpin")->setValue(0.0);
        findChild<QDoubleSpinBox*>("centerYSpin")->setValue(0.0);
        findChild<QDoubleSpinBox*>("centerZSpin")->setValue(0.0);
    });
    connect(resetPose, &QPushButton::clicked, this, [this] {
        findChild<QDoubleSpinBox*>("rotateZSpin")->setValue(0.0);
        findChild<QDoubleSpinBox*>("rotateXSpin")->setValue(0.0);
    });
}

void ProcessingSettingsDialog::buildAdvancedPage()
{
    auto* warning = new QLabel(
        QString::fromUtf8("高级参数会影响点云质量、网格质量和处理性能，请谨慎修改"),
        ui_->advancedPage);
    warning->setObjectName("advancedWarningLabel");
    warning->setFrameShape(QFrame::StyledPanel);
    warning->setMinimumHeight(34);
    ui_->advancedPageLayout->addWidget(warning);

    auto* pointGroup = new QGroupBox(QString::fromUtf8("点云生成"), ui_->advancedPage);
    auto* pointLayout = new QFormLayout(pointGroup);
    auto* preset = new QComboBox(pointGroup);
    preset->setObjectName("calibrationPresetCombo");
    preset->addItem(QString::fromUtf8("默认（084C）"));
    pointLayout->addRow(QString::fromUtf8("标定方案"), preset);
    pointLayout->addRow(QString::fromUtf8("焦距"),
        decimalSpin(pointGroup, "pointCloudFocusSpin", 0.001, 100000.0, 3, 1.0));
    pointLayout->addRow(QString::fromUtf8("视差基线"),
        decimalSpin(pointGroup, "pointCloudDispSpin", 0.0001, 1000.0, 4, 0.1));
    pointLayout->addRow(QString::fromUtf8("视差步长"),
        decimalSpin(pointGroup, "pointCloudStepSpin", 0.0001, 1000.0, 4, 0.01));
    auto* outlier = new QCheckBox(QString::fromUtf8("启用离群点过滤"), pointGroup);
    outlier->setObjectName("outlierFilterCheck");
    pointLayout->addRow(QString(), outlier);
    auto* pointDetails = new QPushButton(QString::fromUtf8("详细参数..."), pointGroup);
    pointDetails->setObjectName("pointCloudDetailsButton");
    pointLayout->addRow(QString(), pointDetails);
    ui_->advancedPageLayout->addWidget(pointGroup);

    auto* meshGroup = new QGroupBox(QString::fromUtf8("网格重建"), ui_->advancedPage);
    auto* meshLayout = new QFormLayout(meshGroup);
    auto* reconstruct = new QComboBox(meshGroup);
    reconstruct->setObjectName("reconstructCombo");
    reconstruct->addItem(QString::fromUtf8("泊松重建"), 1);
    reconstruct->addItem(QString::fromUtf8("贪婪三角化"), 2);
    meshLayout->addRow(QString::fromUtf8("重建算法"), reconstruct);
    meshLayout->addRow(QString::fromUtf8("搜索半径"),
        decimalSpin(meshGroup, "meshSearchRadiusSpin", 0.000001, 1000.0, 6, 0.001));
    meshLayout->addRow(QString::fromUtf8("邻居数量"),
        integerSpin(meshGroup, "meshKSearchSpin", 1, 100000));
    meshLayout->addRow(QString::fromUtf8("体素大小"),
        decimalSpin(meshGroup, "meshLeafSizeSpin", 0.0, 1000.0, 6, 0.001));
    auto* meshDetails = new QPushButton(QString::fromUtf8("详细参数..."), meshGroup);
    meshDetails->setObjectName("meshDetailsButton");
    meshLayout->addRow(QString(), meshDetails);
    ui_->advancedPageLayout->addWidget(meshGroup);

    auto* performanceGroup = new QGroupBox(QString::fromUtf8("多视图与性能"), ui_->advancedPage);
    auto* performanceLayout = new QFormLayout(performanceGroup);
    auto* atlas = integerSpin(performanceGroup, "atlasSizeSpin", 512, 32768);
    atlas->setSingleStep(512);
    performanceLayout->addRow(QString::fromUtf8("纹理图集大小"), atlas);
    performanceLayout->addRow(QString::fromUtf8("处理线程数"),
        integerSpin(performanceGroup, "writerThreadsSpin", 1, 256));
    auto* adaptive = new QCheckBox(QString::fromUtf8("启用硬件自适应"), performanceGroup);
    adaptive->setObjectName("hardwareAdaptiveCheck");
    performanceLayout->addRow(QString(), adaptive);
    ui_->advancedPageLayout->addWidget(performanceGroup);
    ui_->advancedPageLayout->addStretch();

    connect(adaptive, &QCheckBox::toggled,
        this, &ProcessingSettingsDialog::updateHardwareAdaptiveState);
    connect(pointDetails, &QPushButton::clicked,
        this, &ProcessingSettingsDialog::openPointCloudDetails);
    connect(meshDetails, &QPushButton::clicked,
        this, &ProcessingSettingsDialog::openMeshDetails);
}

void ProcessingSettingsDialog::buildDevicePage()
{
    auto* connectionGroup = new QGroupBox(QString::fromUtf8("相机连接"), ui_->devicePage);
    auto* connectionLayout = new QFormLayout(connectionGroup);
    auto* preset = new QComboBox(connectionGroup);
    preset->setObjectName("cameraPresetCombo");
    preset->addItem(QString::fromUtf8("084C 默认方案"));
    connectionLayout->addRow(QString::fromUtf8("配置方案"), preset);

    auto* directoryRow = new QWidget(connectionGroup);
    auto* directoryLayout = new QHBoxLayout(directoryRow);
    directoryLayout->setContentsMargins(0, 0, 0, 0);
    directoryLayout->addWidget(pathEdit(directoryRow, "cameraConfigDirectoryEdit"), 1);
    directoryLayout->addWidget(browseButton(directoryRow, "cameraConfigBrowseButton"));
    connectionLayout->addRow(QString::fromUtf8("配置目录"), directoryRow);
    auto* status = new QLabel(QString::fromUtf8("尚未检查相机配置"), connectionGroup);
    status->setObjectName("cameraStatusLabel");
    connectionLayout->addRow(QString(), status);

    auto* connectionButtons = new QWidget(connectionGroup);
    auto* connectionButtonsLayout = new QHBoxLayout(connectionButtons);
    connectionButtonsLayout->setContentsMargins(0, 0, 0, 0);
    auto* testButton = new QPushButton(QString::fromUtf8("测试连接"), connectionButtons);
    testButton->setObjectName("testCameraButton");
    auto* reinitializeButton = new QPushButton(QString::fromUtf8("重新初始化"), connectionButtons);
    reinitializeButton->setObjectName("reinitializeCameraButton");
    connectionButtonsLayout->addWidget(testButton);
    connectionButtonsLayout->addWidget(reinitializeButton);
    connectionButtonsLayout->addStretch();
    connectionLayout->addRow(QString(), connectionButtons);
    ui_->devicePageLayout->addWidget(connectionGroup);

    auto* captureGroup = new QGroupBox(QString::fromUtf8("采集参数"), ui_->devicePage);
    auto* captureLayout = new QFormLayout(captureGroup);
    auto* exposureMode = new QComboBox(captureGroup);
    exposureMode->setObjectName("cameraExposureModeCombo");
    exposureMode->addItem(QString::fromUtf8("手动"), 1);
    captureLayout->addRow(QString::fromUtf8("曝光模式"), exposureMode);
    captureLayout->addRow(QString::fromUtf8("曝光值"),
        integerSpin(captureGroup, "cameraExposureValueSpin", 0, 2147483647));
    auto* frameRate = decimalSpin(captureGroup, "cameraFrameRateSpin", 0.1, 1000.0, 1, 0.5);
    frameRate->setSuffix(QString::fromUtf8(" fps"));
    captureLayout->addRow(QString::fromUtf8("帧率"), frameRate);
    auto* rotation = new QComboBox(captureGroup);
    rotation->setObjectName("cameraRotationCombo");
    rotation->addItem(QString::fromUtf8("顺时针 90°（当前倒装）"),
        static_cast<int>(CaptureRotation::Clockwise90));
    rotation->addItem(QString::fromUtf8("逆时针 90°（原安装）"),
        static_cast<int>(CaptureRotation::CounterClockwise90));
    captureLayout->addRow(QString::fromUtf8("画面方向"), rotation);
    captureLayout->addRow(QString(),
        new QLabel(QString::fromUtf8("修改后需要重新初始化相机"), captureGroup));
    ui_->devicePageLayout->addWidget(captureGroup);

    auto* infoGroup = new QGroupBox(QString::fromUtf8("相机信息"), ui_->devicePage);
    auto* infoLayout = new QFormLayout(infoGroup);
    auto addInfoRow = [infoGroup, infoLayout](const QString& label, const char* name) {
        QLineEdit* edit = pathEdit(infoGroup, name);
        infoLayout->addRow(label, edit);
    };
    addInfoRow(QString::fromUtf8("相机接口"), "cameraInterfaceEdit");
    addInfoRow(QString::fromUtf8("相机类型"), "cameraTypeEdit");
    addInfoRow(QString::fromUtf8("相机编号"), "cameraIdEdit");
    addInfoRow(QString::fromUtf8("GPU"), "cameraGpuEdit");
    auto* engineer = new QPushButton(QString::fromUtf8("工程师设置..."), infoGroup);
    engineer->setObjectName("engineerSettingsButton");
    infoLayout->addRow(QString(), engineer);
    ui_->devicePageLayout->addWidget(infoGroup);
    auto* idleHint = new QLabel(
        QString::fromUtf8("设备设置仅在空闲状态下可以修改"), ui_->devicePage);
    idleHint->setAlignment(Qt::AlignRight);
    ui_->devicePageLayout->addWidget(idleHint);
    ui_->devicePageLayout->addStretch();

    connect(findChild<QPushButton*>("cameraConfigBrowseButton"), &QPushButton::clicked,
        this, &ProcessingSettingsDialog::browseCameraDirectory);
    connect(engineer, &QPushButton::clicked,
        this, &ProcessingSettingsDialog::openEngineerSettings);
    connect(testButton, &QPushButton::clicked, this, [this] {
        const QDir directory(findChild<QLineEdit*>("cameraConfigDirectoryEdit")->text());
        const bool valid = directory.exists()
            && QFileInfo(directory.filePath("jp.xml")).isFile()
            && QFileInfo(directory.filePath("param.txt")).isFile()
            && !directory.entryList({ "*.cen" }, QDir::Files).isEmpty();
        if (!valid) {
            setCameraTestResult(false, QString::fromUtf8("配置目录无效，请检查标定文件"));
            return;
        }
        findChild<QLabel*>("cameraStatusLabel")->setText(
            QString::fromUtf8("正在测试相机连接..."));
        emit cameraTestRequested(settings().camera);
    });
    connect(reinitializeButton, &QPushButton::clicked, this, [this] {
        emit cameraReinitializeRequested(settings().camera);
    });
}

void ProcessingSettingsDialog::populateImagingPage()
{
    const PipelineUiSettings& p = draft_.pipeline;
    findChild<QDoubleSpinBox*>("subjectSizeSpin")->setValue(p.subjectSize);
    findChild<QDoubleSpinBox*>("centerXSpin")->setValue(p.centerX);
    findChild<QDoubleSpinBox*>("centerYSpin")->setValue(p.centerY);
    findChild<QDoubleSpinBox*>("centerZSpin")->setValue(p.centerZ);
    findChild<QDoubleSpinBox*>("rotateZSpin")->setValue(p.rotateZDeg);
    findChild<QDoubleSpinBox*>("rotateXSpin")->setValue(p.rotateXDeg);
    findChild<QSpinBox*>("jpgQualitySpin")->setValue(p.jpgQuality);
    findChild<QCheckBox*>("captureFlipCheck")->setChecked(p.captureFlipVertical);
    const int direction = (p.elementalFlipSourceY ? 1 : 0)
        + (p.elementalFlipViewRows ? 2 : 0);
    findChild<QComboBox*>("elementalDirectionCombo")->setCurrentIndex(direction);
}

void ProcessingSettingsDialog::populateAdvancedPage()
{
    findChild<QDoubleSpinBox*>("pointCloudFocusSpin")->setValue(draft_.pointCloud.focus);
    findChild<QDoubleSpinBox*>("pointCloudDispSpin")->setValue(draft_.pointCloud.disp);
    findChild<QDoubleSpinBox*>("pointCloudStepSpin")->setValue(draft_.pointCloud.step);
    findChild<QCheckBox*>("outlierFilterCheck")->setChecked(draft_.pointCloud.outlierFilterEnabled);
    QComboBox* reconstruct = findChild<QComboBox*>("reconstructCombo");
    reconstruct->setCurrentIndex(std::max(0, reconstruct->findData(draft_.mesh.reconstruct)));
    findChild<QDoubleSpinBox*>("meshSearchRadiusSpin")->setValue(draft_.mesh.searchRadius);
    findChild<QSpinBox*>("meshKSearchSpin")->setValue(draft_.mesh.kSearch);
    findChild<QDoubleSpinBox*>("meshLeafSizeSpin")->setValue(draft_.mesh.leafSize);
    const bool adaptive = draft_.pipeline.atlasSize == 0 && draft_.pipeline.writerThreads == 0;
    findChild<QCheckBox*>("hardwareAdaptiveCheck")->setChecked(adaptive);
    findChild<QSpinBox*>("atlasSizeSpin")->setValue(
        adaptive ? 4096 : std::max(512, draft_.pipeline.atlasSize));
    findChild<QSpinBox*>("writerThreadsSpin")->setValue(
        adaptive ? 1 : std::max(1, draft_.pipeline.writerThreads));
    updateHardwareAdaptiveState();
}

void ProcessingSettingsDialog::populateDevicePage()
{
    const CameraCaptureSettings& camera = draft_.camera;
    findChild<QLineEdit*>("cameraConfigDirectoryEdit")->setText(
        QDir::toNativeSeparators(camera.configDirectory));
    QComboBox* exposureMode = findChild<QComboBox*>("cameraExposureModeCombo");
    exposureMode->setCurrentIndex(std::max(0, exposureMode->findData(camera.exposureMode)));
    findChild<QSpinBox*>("cameraExposureValueSpin")->setValue(camera.exposureValue);
    findChild<QDoubleSpinBox*>("cameraFrameRateSpin")->setValue(camera.frameRate);
    QComboBox* rotation = findChild<QComboBox*>("cameraRotationCombo");
    rotation->setCurrentIndex(std::max(0,
        rotation->findData(static_cast<int>(camera.rotation))));
    findChild<QLineEdit*>("cameraInterfaceEdit")->setText(camera.cameraInterface);
    findChild<QLineEdit*>("cameraTypeEdit")->setText(camera.cameraType);
    findChild<QLineEdit*>("cameraIdEdit")->setText(QString::number(camera.cameraId));
    findChild<QLineEdit*>("cameraGpuEdit")->setText(QString::number(camera.gpuId));
    findChild<QLabel*>("cameraStatusLabel")->setText(QString::fromUtf8("配置目录待应用"));
}

void ProcessingSettingsDialog::collectImagingPage(ProcessingSettings* settings) const
{
    PipelineUiSettings& p = settings->pipeline;
    p.subjectSize = findChild<QDoubleSpinBox*>("subjectSizeSpin")->value();
    p.centerX = findChild<QDoubleSpinBox*>("centerXSpin")->value();
    p.centerY = findChild<QDoubleSpinBox*>("centerYSpin")->value();
    p.centerZ = findChild<QDoubleSpinBox*>("centerZSpin")->value();
    p.rotateZDeg = findChild<QDoubleSpinBox*>("rotateZSpin")->value();
    p.rotateXDeg = findChild<QDoubleSpinBox*>("rotateXSpin")->value();
    p.jpgQuality = findChild<QSpinBox*>("jpgQualitySpin")->value();
    p.captureFlipVertical = findChild<QCheckBox*>("captureFlipCheck")->isChecked();
    const int direction = findChild<QComboBox*>("elementalDirectionCombo")->currentIndex();
    p.elementalFlipSourceY = (direction & 1) != 0;
    p.elementalFlipViewRows = (direction & 2) != 0;
}

void ProcessingSettingsDialog::collectAdvancedPage(ProcessingSettings* settings) const
{
    settings->pointCloud.focus = findChild<QDoubleSpinBox*>("pointCloudFocusSpin")->value();
    settings->pointCloud.disp = findChild<QDoubleSpinBox*>("pointCloudDispSpin")->value();
    settings->pointCloud.step = findChild<QDoubleSpinBox*>("pointCloudStepSpin")->value();
    settings->pointCloud.outlierFilterEnabled = findChild<QCheckBox*>("outlierFilterCheck")->isChecked();
    settings->mesh.reconstruct = findChild<QComboBox*>("reconstructCombo")->currentData().toInt();
    settings->mesh.searchRadius = findChild<QDoubleSpinBox*>("meshSearchRadiusSpin")->value();
    settings->mesh.kSearch = findChild<QSpinBox*>("meshKSearchSpin")->value();
    settings->mesh.leafSize = findChild<QDoubleSpinBox*>("meshLeafSizeSpin")->value();
    const bool adaptive = findChild<QCheckBox*>("hardwareAdaptiveCheck")->isChecked();
    settings->pipeline.atlasSize = adaptive ? 0 : findChild<QSpinBox*>("atlasSizeSpin")->value();
    settings->pipeline.writerThreads = adaptive ? 0 : findChild<QSpinBox*>("writerThreadsSpin")->value();
}

void ProcessingSettingsDialog::collectDevicePage(ProcessingSettings* settings) const
{
    CameraCaptureSettings& camera = settings->camera;
    camera.configDirectory = findChild<QLineEdit*>("cameraConfigDirectoryEdit")->text();
    camera.exposureMode = findChild<QComboBox*>("cameraExposureModeCombo")->currentData().toInt();
    camera.exposureValue = findChild<QSpinBox*>("cameraExposureValueSpin")->value();
    camera.frameRate = findChild<QDoubleSpinBox*>("cameraFrameRateSpin")->value();
    camera.rotation = static_cast<CaptureRotation>(
        findChild<QComboBox*>("cameraRotationCombo")->currentData().toInt());
    camera.cameraInterface = findChild<QLineEdit*>("cameraInterfaceEdit")->text();
    camera.cameraType = findChild<QLineEdit*>("cameraTypeEdit")->text();
    camera.cameraId = findChild<QLineEdit*>("cameraIdEdit")->text().toInt();
    camera.gpuId = findChild<QLineEdit*>("cameraGpuEdit")->text().toInt();
}

void ProcessingSettingsDialog::updateHardwareAdaptiveState()
{
    const bool manual = !findChild<QCheckBox*>("hardwareAdaptiveCheck")->isChecked();
    findChild<QSpinBox*>("atlasSizeSpin")->setEnabled(manual);
    findChild<QSpinBox*>("writerThreadsSpin")->setEnabled(manual);
}

void ProcessingSettingsDialog::populateCommonPage()
{
    QComboBox* inputMode = findChild<QComboBox*>("inputModeCombo");
    const int modeIndex = inputMode->findData(static_cast<int>(draft_.input.mode));
    inputMode->setCurrentIndex(std::max(0, modeIndex));
    findChild<QLineEdit*>("inputDirectoryEdit")->setText(displayPath(draft_.input.directory));
    findChild<QLineEdit*>("outputDirectoryEdit")->setText(
        QDir::toNativeSeparators(draft_.pipeline.outputRoot));
    findChild<QSpinBox*>("multiviewAngleSpin")->setValue(draft_.pipeline.multiviewAngle);
    findChild<QSpinBox*>("multiviewPerSpin")->setValue(draft_.pipeline.multiviewPer);
    findChild<QSpinBox*>("multiviewResolutionSpin")->setValue(draft_.pipeline.multiviewResolution);
    findChild<QSpinBox*>("targetRowsSpin")->setValue(draft_.pipeline.targetRows);
    findChild<QSpinBox*>("targetColsSpin")->setValue(draft_.pipeline.targetCols);
    findChild<QCheckBox*>("saveMeshCheck")->setChecked(draft_.saveResults.mesh);
    findChild<QCheckBox*>("saveMultiviewCheck")->setChecked(draft_.saveResults.multiview);
    findChild<QCheckBox*>("saveElementalCheck")->setChecked(draft_.saveResults.elemental);
    updateInputState();
    updateDerivedSummary();
}

void ProcessingSettingsDialog::collectCommonPage(ProcessingSettings* settings) const
{
    const QComboBox* inputMode = findChild<QComboBox*>("inputModeCombo");
    settings->input.mode = static_cast<PipelineInputMode>(inputMode->currentData().toInt());
    const QString inputDirectory = findChild<QLineEdit*>("inputDirectoryEdit")->text();
    settings->input.directory = settings->input.mode == PipelineInputMode::Camera
        ? std::filesystem::path{} : filesystemPath(inputDirectory);
    settings->pipeline.outputRoot = findChild<QLineEdit*>("outputDirectoryEdit")->text();
    settings->pipeline.multiviewAngle = findChild<QSpinBox*>("multiviewAngleSpin")->value();
    settings->pipeline.multiviewPer = findChild<QSpinBox*>("multiviewPerSpin")->value();
    settings->pipeline.multiviewResolution = findChild<QSpinBox*>("multiviewResolutionSpin")->value();
    settings->pipeline.targetRows = findChild<QSpinBox*>("targetRowsSpin")->value();
    settings->pipeline.targetCols = findChild<QSpinBox*>("targetColsSpin")->value();
    settings->saveResults.mesh = findChild<QCheckBox*>("saveMeshCheck")->isChecked();
    settings->saveResults.multiview = findChild<QCheckBox*>("saveMultiviewCheck")->isChecked();
    settings->saveResults.elemental = findChild<QCheckBox*>("saveElementalCheck")->isChecked();
}

void ProcessingSettingsDialog::updateInputState()
{
    const bool external = findChild<QComboBox*>("inputModeCombo")->currentData().toInt()
        != static_cast<int>(PipelineInputMode::Camera);
    QLineEdit* edit = findChild<QLineEdit*>("inputDirectoryEdit");
    QPushButton* browse = findChild<QPushButton*>("inputBrowseButton");
    edit->setEnabled(external);
    browse->setEnabled(external);
    if (!external) {
        edit->setText(QString::fromUtf8("使用实时相机"));
    }
    else if (edit->text() == QString::fromUtf8("使用实时相机")) {
        edit->clear();
    }
}

void ProcessingSettingsDialog::updateDerivedSummary()
{
    const int views = findChild<QSpinBox*>("multiviewAngleSpin")->value()
        * findChild<QSpinBox*>("multiviewPerSpin")->value();
    const qint64 elemental = static_cast<qint64>(findChild<QSpinBox*>("targetRowsSpin")->value())
        * findChild<QSpinBox*>("targetColsSpin")->value();
    findChild<QLabel*>("derivedSummaryLabel")->setText(
        QString::fromUtf8("将生成 %1 × %1 个视点，%2 张 Elemental 图像")
            .arg(views).arg(elemental));
}

void ProcessingSettingsDialog::browseInputDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this, QString::fromUtf8("选择输入目录"), QDir::homePath());
    if (!directory.isEmpty()) {
        findChild<QLineEdit*>("inputDirectoryEdit")->setText(QDir::toNativeSeparators(directory));
    }
}

void ProcessingSettingsDialog::browseOutputDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this, QString::fromUtf8("选择输出目录"),
        findChild<QLineEdit*>("outputDirectoryEdit")->text());
    if (!directory.isEmpty()) {
        findChild<QLineEdit*>("outputDirectoryEdit")->setText(QDir::toNativeSeparators(directory));
    }
}

void ProcessingSettingsDialog::browseCameraDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this, QString::fromUtf8("选择相机配置目录"),
        findChild<QLineEdit*>("cameraConfigDirectoryEdit")->text());
    if (!directory.isEmpty()) {
        findChild<QLineEdit*>("cameraConfigDirectoryEdit")->setText(
            QDir::toNativeSeparators(directory));
        findChild<QLabel*>("cameraStatusLabel")->setText(
            QString::fromUtf8("配置目录已更改，尚未检查"));
    }
}

void ProcessingSettingsDialog::openPointCloudDetails()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QString::fromUtf8("点云详细参数"));
    auto* root = new QVBoxLayout(&dialog);
    auto* hint = new QLabel(
        QString::fromUtf8("这些参数用于深度标定与离群点过滤，通常保持标定方案默认值。"),
        &dialog);
    hint->setWordWrap(true);
    root->addWidget(hint);
    auto* form = new QFormLayout;
    auto* label = integerSpin(&dialog, "pointDetailLabelSpin", 1, 65535);
    auto* fdis = decimalSpin(&dialog, "pointDetailFdisSpin", 0.0, 10000.0, 4, 0.1);
    auto* greenRgb = integerSpin(&dialog, "pointDetailGreenRgbSpin", 0, 255);
    auto* meanK = integerSpin(&dialog, "pointDetailMeanKSpin", 1, 100000);
    auto* stddev = decimalSpin(&dialog, "pointDetailStddevSpin", 0.0, 1000.0, 4, 0.1);
    auto* radius = decimalSpin(&dialog, "pointDetailRadiusSpin", 0.000001, 1000.0, 6, 0.001);
    auto* minNeighbors = integerSpin(&dialog, "pointDetailMinNeighborsSpin", 1, 100000);
    label->setValue(draft_.pointCloud.label);
    fdis->setValue(draft_.pointCloud.fdis);
    greenRgb->setValue(draft_.pointCloud.greenRgb);
    meanK->setValue(draft_.pointCloud.meanK);
    stddev->setValue(draft_.pointCloud.stddevMulThreshold);
    radius->setValue(draft_.pointCloud.radiusSearch);
    minNeighbors->setValue(draft_.pointCloud.minNeighborsInRadius);
    form->addRow(QString::fromUtf8("最大标签值"), label);
    form->addRow(QString::fromUtf8("视差像素偏移"), fdis);
    form->addRow(QString::fromUtf8("绿色背景阈值"), greenRgb);
    form->addRow(QString::fromUtf8("统计邻居数"), meanK);
    form->addRow(QString::fromUtf8("标准差倍数"), stddev);
    form->addRow(QString::fromUtf8("半径过滤范围"), radius);
    form->addRow(QString::fromUtf8("半径内最少邻居"), minNeighbors);
    root->addLayout(form);
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("取消"));
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    root->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    draft_.pointCloud.label = label->value();
    draft_.pointCloud.fdis = fdis->value();
    draft_.pointCloud.greenRgb = greenRgb->value();
    draft_.pointCloud.meanK = meanK->value();
    draft_.pointCloud.stddevMulThreshold = stddev->value();
    draft_.pointCloud.radiusSearch = radius->value();
    draft_.pointCloud.minNeighborsInRadius = minNeighbors->value();
}

void ProcessingSettingsDialog::openMeshDetails()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QString::fromUtf8("网格详细参数"));
    auto* root = new QVBoxLayout(&dialog);
    auto* hint = new QLabel(
        QString::fromUtf8("灰色项目不用于当前重建算法；切换算法后再修改对应参数。"),
        &dialog);
    hint->setWordWrap(true);
    root->addWidget(hint);
    auto* form = new QFormLayout;
    auto* mu = decimalSpin(&dialog, "meshDetailMuSpin", 0.0, 1000.0, 4, 0.1);
    auto* maximumNeighbors = integerSpin(&dialog, "meshDetailMaximumNeighborsSpin", 1, 100000);
    auto* surfaceAngle = decimalSpin(&dialog, "meshDetailSurfaceAngleSpin", 0.0, 180.0, 2, 1.0);
    auto* minimumAngle = decimalSpin(&dialog, "meshDetailMinimumAngleSpin", 0.0, 180.0, 2, 1.0);
    auto* maximumAngle = decimalSpin(&dialog, "meshDetailMaximumAngleSpin", 0.0, 180.0, 2, 1.0);
    auto* holeSize = decimalSpin(&dialog, "meshDetailHoleSizeSpin", 0.0, 1000.0, 6, 0.001);
    auto* textureFocus = decimalSpin(&dialog, "meshDetailTextureFocusSpin", 0.001, 100000.0, 3, 1.0);
    auto* mlsRadius = decimalSpin(&dialog, "meshDetailMlsRadiusSpin", 0.0, 1000.0, 6, 0.001);
    auto* iterations1 = integerSpin(&dialog, "meshDetailIterations1Spin", 0, 100000);
    auto* iterations2 = integerSpin(&dialog, "meshDetailIterations2Spin", 0, 100000);
    auto* neighborCount = integerSpin(&dialog, "meshDetailNeighborCountSpin", 1, 100000);
    auto* nearestDistance = decimalSpin(&dialog, "meshDetailNearestDistanceSpin", 0.0, 1000.0, 6, 0.001);
    mu->setValue(draft_.mesh.mu);
    maximumNeighbors->setValue(draft_.mesh.maximumNearestNeighbors);
    surfaceAngle->setValue(draft_.mesh.maximumSurfaceAngle);
    minimumAngle->setValue(draft_.mesh.minimumAngle);
    maximumAngle->setValue(draft_.mesh.maximumAngle);
    holeSize->setValue(draft_.mesh.holeSize);
    textureFocus->setValue(draft_.mesh.textureFocus);
    mlsRadius->setValue(draft_.mesh.mlsSearchRadius);
    iterations1->setValue(draft_.mesh.normalsFitIterations1);
    iterations2->setValue(draft_.mesh.normalsFitIterations2);
    neighborCount->setValue(draft_.mesh.neighborCount);
    nearestDistance->setValue(draft_.mesh.nearestDistance);
    form->addRow(QString::fromUtf8("最近邻距离倍数"), mu);
    form->addRow(QString::fromUtf8("最大最近邻数"), maximumNeighbors);
    form->addRow(QString::fromUtf8("最大表面角"), surfaceAngle);
    form->addRow(QString::fromUtf8("最小三角角度"), minimumAngle);
    form->addRow(QString::fromUtf8("最大三角角度"), maximumAngle);
    form->addRow(QString::fromUtf8("孔洞尺寸"), holeSize);
    form->addRow(QString::fromUtf8("纹理焦距"), textureFocus);
    form->addRow(QString::fromUtf8("MLS 搜索半径"), mlsRadius);
    form->addRow(QString::fromUtf8("法线拟合迭代 1"), iterations1);
    form->addRow(QString::fromUtf8("法线拟合迭代 2"), iterations2);
    form->addRow(QString::fromUtf8("法线邻居数"), neighborCount);
    form->addRow(QString::fromUtf8("最近邻距离"), nearestDistance);
    root->addLayout(form);
    const bool greedy = findChild<QComboBox*>("reconstructCombo")->currentData().toInt() == 2;
    QWidget* greedyControls[] = {
        mu, maximumNeighbors, surfaceAngle, minimumAngle, maximumAngle
    };
    for (QWidget* control : greedyControls) {
        control->setEnabled(greedy);
    }
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("取消"));
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    root->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    draft_.mesh.mu = mu->value();
    draft_.mesh.maximumNearestNeighbors = maximumNeighbors->value();
    draft_.mesh.maximumSurfaceAngle = surfaceAngle->value();
    draft_.mesh.minimumAngle = minimumAngle->value();
    draft_.mesh.maximumAngle = maximumAngle->value();
    draft_.mesh.holeSize = holeSize->value();
    draft_.mesh.textureFocus = textureFocus->value();
    draft_.mesh.mlsSearchRadius = mlsRadius->value();
    draft_.mesh.normalsFitIterations1 = iterations1->value();
    draft_.mesh.normalsFitIterations2 = iterations2->value();
    draft_.mesh.neighborCount = neighborCount->value();
    draft_.mesh.nearestDistance = nearestDistance->value();
}

void ProcessingSettingsDialog::openEngineerSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QString::fromUtf8("相机工程师设置"));
    auto* root = new QVBoxLayout(&dialog);
    auto* warning = new QLabel(
        QString::fromUtf8("这些参数直接影响相机驱动和数据解析，请确认设备资料后修改。"),
        &dialog);
    warning->setWordWrap(true);
    root->addWidget(warning);
    auto* form = new QFormLayout;
    auto* cameraInterface = new QLineEdit(
        findChild<QLineEdit*>("cameraInterfaceEdit")->text(), &dialog);
    auto* cameraType = new QLineEdit(
        findChild<QLineEdit*>("cameraTypeEdit")->text(), &dialog);
    auto* cameraId = integerSpin(&dialog, "engineerCameraIdSpin", 0, 1024);
    cameraId->setValue(findChild<QLineEdit*>("cameraIdEdit")->text().toInt());
    auto* gpuId = integerSpin(&dialog, "engineerGpuIdSpin", 0, 64);
    gpuId->setValue(findChild<QLineEdit*>("cameraGpuEdit")->text().toInt());
    auto* missed = integerSpin(&dialog, "engineerMissedFrameSpin", 0, 1000000);
    missed->setValue(draft_.camera.missedFrameThreshold);
    form->addRow(QString::fromUtf8("相机接口"), cameraInterface);
    form->addRow(QString::fromUtf8("相机类型"), cameraType);
    form->addRow(QString::fromUtf8("相机编号"), cameraId);
    form->addRow(QString::fromUtf8("GPU"), gpuId);
    form->addRow(QString::fromUtf8("丢帧阈值"), missed);
    root->addLayout(form);
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("取消"));
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    root->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    findChild<QLineEdit*>("cameraInterfaceEdit")->setText(cameraInterface->text().trimmed());
    findChild<QLineEdit*>("cameraTypeEdit")->setText(cameraType->text().trimmed());
    findChild<QLineEdit*>("cameraIdEdit")->setText(QString::number(cameraId->value()));
    findChild<QLineEdit*>("cameraGpuEdit")->setText(QString::number(gpuId->value()));
    draft_.camera.missedFrameThreshold = missed->value();
}

void ProcessingSettingsDialog::restoreCurrentPageDefaults()
{
    const QString outputRoot = draft_.pipeline.outputRoot;
    const QString cameraDirectory = draft_.camera.configDirectory;
    ProcessingSettings defaults = defaultProcessingSettings(QDir::currentPath(), cameraDirectory);
    defaults.pipeline.outputRoot = outputRoot;
    if (selectedPage_ == 0) {
        draft_.input.clear();
        draft_.saveResults = defaults.saveResults;
        draft_.pipeline.multiviewAngle = defaults.pipeline.multiviewAngle;
        draft_.pipeline.multiviewPer = defaults.pipeline.multiviewPer;
        draft_.pipeline.multiviewResolution = defaults.pipeline.multiviewResolution;
        draft_.pipeline.targetRows = defaults.pipeline.targetRows;
        draft_.pipeline.targetCols = defaults.pipeline.targetCols;
        populateCommonPage();
    }
    else if (selectedPage_ == 1) {
        draft_.pipeline.subjectSize = defaults.pipeline.subjectSize;
        draft_.pipeline.centerX = defaults.pipeline.centerX;
        draft_.pipeline.centerY = defaults.pipeline.centerY;
        draft_.pipeline.centerZ = defaults.pipeline.centerZ;
        draft_.pipeline.rotateXDeg = defaults.pipeline.rotateXDeg;
        draft_.pipeline.rotateZDeg = defaults.pipeline.rotateZDeg;
        draft_.pipeline.jpgQuality = defaults.pipeline.jpgQuality;
        draft_.pipeline.captureFlipVertical = defaults.pipeline.captureFlipVertical;
        draft_.pipeline.elementalFlipSourceY = defaults.pipeline.elementalFlipSourceY;
        draft_.pipeline.elementalFlipViewRows = defaults.pipeline.elementalFlipViewRows;
        populateImagingPage();
    }
    else if (selectedPage_ == 2) {
        draft_.pointCloud = defaults.pointCloud;
        draft_.mesh = defaults.mesh;
        draft_.pipeline.atlasSize = defaults.pipeline.atlasSize;
        draft_.pipeline.writerThreads = defaults.pipeline.writerThreads;
        populateAdvancedPage();
    }
    else if (selectedPage_ == 3) {
        draft_.camera = defaults.camera;
        populateDevicePage();
    }
}

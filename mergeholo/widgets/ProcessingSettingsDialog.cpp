#include "ProcessingSettingsDialog.h"

#include "ui_ProcessingSettingsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
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
}

ProcessingSettings ProcessingSettingsDialog::settings() const
{
    ProcessingSettings result = draft_;
    collectCommonPage(&result);
    collectImagingPage(&result);
    collectAdvancedPage(&result);
    return result;
}

void ProcessingSettingsDialog::setBusy(bool busy)
{
    busy_ = busy;
    ui_->devicePage->setEnabled(!busy);
    ui_->applyButton->setEnabled(!busy);
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
    ui_->devicePageLayout->addWidget(new QLabel(
        QString::fromUtf8("相机连接和采集参数将在此页设置。"), ui_->devicePage));
    ui_->devicePageLayout->addStretch();
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
}

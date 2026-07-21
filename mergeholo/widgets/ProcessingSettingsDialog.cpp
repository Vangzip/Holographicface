#include "ProcessingSettingsDialog.h"

#include "ui_ProcessingSettingsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
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
}

ProcessingSettings ProcessingSettingsDialog::settings() const
{
    ProcessingSettings result = draft_;
    collectCommonPage(&result);
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
    ui_->imagingPageLayout->addWidget(new QLabel(
        QString::fromUtf8("成像参数将在此页设置。"), ui_->imagingPage));
    ui_->imagingPageLayout->addStretch();
    ui_->advancedPageLayout->addWidget(new QLabel(
        QString::fromUtf8("点云、网格和性能参数将在此页设置。"), ui_->advancedPage));
    ui_->advancedPageLayout->addStretch();
    ui_->devicePageLayout->addWidget(new QLabel(
        QString::fromUtf8("相机连接和采集参数将在此页设置。"), ui_->devicePage));
    ui_->devicePageLayout->addStretch();
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
    if (selectedPage_ != 0) {
        return;
    }
    const QString outputRoot = draft_.pipeline.outputRoot;
    const QString cameraDirectory = draft_.camera.configDirectory;
    ProcessingSettings defaults = defaultProcessingSettings(QDir::currentPath(), cameraDirectory);
    defaults.pipeline.outputRoot = outputRoot;
    draft_.input.clear();
    draft_.saveResults = defaults.saveResults;
    draft_.pipeline = defaults.pipeline;
    populateCommonPage();
}

#include "InputSettingsDialog.h"

#include "ui_InputSettingsDialog.h"

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

namespace {

std::filesystem::path filesystemPath(const QString& path)
{
#ifdef _WIN32
    return std::filesystem::path(path.toStdWString()).lexically_normal();
#else
    return std::filesystem::path(path.toStdString()).lexically_normal();
#endif
}

QString displayPath(const std::filesystem::path& path)
{
#ifdef _WIN32
    return QDir::toNativeSeparators(QString::fromStdWString(path.wstring()));
#else
    return QDir::toNativeSeparators(QString::fromStdString(path.string()));
#endif
}

} // namespace

InputSettingsDialog::InputSettingsDialog(QWidget* parent)
    : QDialog(parent)
    , ui_(new Ui::InputSettingsDialog)
{
    ui_->setupUi(this);
    setWindowModality(Qt::ApplicationModal);

    ui_->buttonBox->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("确认"));
    ui_->buttonBox->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("取消"));

    connect(ui_->rgbDepthBrowseButton, &QPushButton::clicked, this, [this] {
        browseDirectory(PipelineInputMode::RgbDepth);
    });
    connect(ui_->meshBrowseButton, &QPushButton::clicked, this, [this] {
        browseDirectory(PipelineInputMode::Mesh);
    });
    connect(ui_->multiviewBrowseButton, &QPushButton::clicked, this, [this] {
        browseDirectory(PipelineInputMode::Multiview);
    });
    connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &InputSettingsDialog::accept);
    connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &InputSettingsDialog::reject);
}

InputSettingsDialog::~InputSettingsDialog() = default;

void InputSettingsDialog::setInputSettings(const PipelineInputSelection& settings)
{
    clearSettings();
    if (settings.isExternal() && !settings.directory.empty()) {
        setDirectory(settings.mode, displayPath(settings.directory));
    }
}

PipelineInputSelection InputSettingsDialog::inputSettings() const
{
    return settings_;
}

void InputSettingsDialog::setDirectory(PipelineInputMode mode, const QString& directory)
{
    clearSettings();
    if (mode == PipelineInputMode::Camera || directory.trimmed().isEmpty()) {
        return;
    }

    const QString absolutePath = QDir::toNativeSeparators(
        QDir(directory).absolutePath());
    settings_.mode = mode;
    settings_.directory = filesystemPath(absolutePath);
    switch (mode) {
    case PipelineInputMode::RgbDepth:
        ui_->rgbDepthPathEdit->setText(absolutePath);
        break;
    case PipelineInputMode::Mesh:
        ui_->meshPathEdit->setText(absolutePath);
        break;
    case PipelineInputMode::Multiview:
        ui_->multiviewPathEdit->setText(absolutePath);
        break;
    case PipelineInputMode::Camera:
    default:
        break;
    }
}

void InputSettingsDialog::accept()
{
    PipelineInputFiles files;
    std::string errorMessage;
    if (!resolvePipelineInput(settings_, MultiviewInputSpec{}, &files, &errorMessage)) {
        QMessageBox::warning(
            this,
            QString::fromUtf8("输入设置无效"),
            QString::fromUtf8(errorMessage.c_str()));
        return;
    }
    QDialog::accept();
}

void InputSettingsDialog::reject()
{
    clearSettings();
    QDialog::reject();
}

void InputSettingsDialog::closeEvent(QCloseEvent* event)
{
    clearSettings();
    QDialog::closeEvent(event);
}

void InputSettingsDialog::browseDirectory(PipelineInputMode mode)
{
    QString initialDirectory = activeDirectory(mode);
    if (initialDirectory.isEmpty()) {
        initialDirectory = QDir::homePath();
    }
    const QString directory = QFileDialog::getExistingDirectory(
        this,
        QString::fromUtf8("选择输入文件夹"),
        initialDirectory,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!directory.isEmpty()) {
        setDirectory(mode, directory);
    }
}

void InputSettingsDialog::clearSettings()
{
    settings_.clear();
    ui_->rgbDepthPathEdit->clear();
    ui_->meshPathEdit->clear();
    ui_->multiviewPathEdit->clear();
}

QString InputSettingsDialog::activeDirectory(PipelineInputMode mode) const
{
    if (settings_.mode != mode) {
        return {};
    }
    return displayPath(settings_.directory);
}

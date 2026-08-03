#include "SaveSettingsDialog.h"

#include "ui_SaveSettingsDialog.h"

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QPushButton>

SaveSettingsDialog::SaveSettingsDialog(QWidget* parent)
    : QDialog(parent)
    , ui_(new Ui::SaveSettingsDialog)
{
    ui_->setupUi(this);
    setWindowModality(Qt::ApplicationModal);

    ui_->buttonBox->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("确定"));
    ui_->buttonBox->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("取消"));

    connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &SaveSettingsDialog::reject);
}

SaveSettingsDialog::~SaveSettingsDialog() = default;

void SaveSettingsDialog::setSaveSettings(const ResultSaveSettings& settings)
{
    ui_->meshCheckBox->setChecked(settings.mesh);
    ui_->multiviewCheckBox->setChecked(settings.multiview);
    ui_->elementalCheckBox->setChecked(settings.elemental);
}

ResultSaveSettings SaveSettingsDialog::saveSettings() const
{
    ResultSaveSettings settings;
    settings.mesh = ui_->meshCheckBox->isChecked();
    settings.multiview = ui_->multiviewCheckBox->isChecked();
    settings.elemental = ui_->elementalCheckBox->isChecked();
    return settings;
}

void SaveSettingsDialog::reject()
{
    clearSettingsAndReject();
}

void SaveSettingsDialog::closeEvent(QCloseEvent* event)
{
    clearSettingsAndReject();
    event->accept();
}

void SaveSettingsDialog::clearSettings()
{
    ui_->meshCheckBox->setChecked(false);
    ui_->multiviewCheckBox->setChecked(false);
    ui_->elementalCheckBox->setChecked(false);
}

void SaveSettingsDialog::clearSettingsAndReject()
{
    clearSettings();
    QDialog::reject();
}

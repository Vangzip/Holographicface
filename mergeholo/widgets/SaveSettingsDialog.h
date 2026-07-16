#pragma once

#include "ResultSaveSettings.h"

#include <QDialog>

#include <memory>

class QCloseEvent;

namespace Ui {
class SaveSettingsDialog;
}

class SaveSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SaveSettingsDialog(QWidget* parent = nullptr);
    ~SaveSettingsDialog() override;

    void setSaveSettings(const ResultSaveSettings& settings);
    ResultSaveSettings saveSettings() const;

public slots:
    void reject() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void clearSettings();
    void clearSettingsAndReject();

    std::unique_ptr<Ui::SaveSettingsDialog> ui_;
};

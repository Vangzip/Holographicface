#pragma once

#include "PipelineInput.h"

#include <QDialog>
#include <QString>

#include <memory>

class QCloseEvent;

namespace Ui {
class InputSettingsDialog;
}

class InputSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InputSettingsDialog(QWidget* parent = nullptr);
    ~InputSettingsDialog() override;

    void setInputSettings(const PipelineInputSelection& settings);
    PipelineInputSelection inputSettings() const;
    void setDirectory(PipelineInputMode mode, const QString& directory);

public slots:
    void accept() override;
    void reject() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void browseDirectory(PipelineInputMode mode);
    void clearSettings();
    QString activeDirectory(PipelineInputMode mode) const;

    std::unique_ptr<Ui::InputSettingsDialog> ui_;
    PipelineInputSelection settings_;
};

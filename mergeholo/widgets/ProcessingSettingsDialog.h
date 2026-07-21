#pragma once

#include "ProcessingSettings.h"

#include <QDialog>

#include <memory>

namespace Ui {
class ProcessingSettingsDialog;
}

class ProcessingSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProcessingSettingsDialog(QWidget* parent = nullptr);
    ~ProcessingSettingsDialog() override;

    void setSettings(const ProcessingSettings& settings);
    ProcessingSettings settings() const;
    void setBusy(bool busy);
    void selectPage(int pageIndex);

public slots:
    void accept() override;

signals:
    void printRequested();

private:
    void buildCommonPage();
    void buildPlaceholderPages();
    void populateCommonPage();
    void collectCommonPage(ProcessingSettings* settings) const;
    void updateInputState();
    void updateDerivedSummary();
    void browseInputDirectory();
    void browseOutputDirectory();
    void restoreCurrentPageDefaults();

    std::unique_ptr<Ui::ProcessingSettingsDialog> ui_;
    ProcessingSettings draft_;
    int selectedPage_ = 0;
    bool busy_ = false;
};

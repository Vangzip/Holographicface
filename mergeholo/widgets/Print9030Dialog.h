#pragma once

#include "PrintConfig.h"
#include "PrintImageSource.h"
#include "elemental/ElementalMemoryResult.h"

#include <QDialog>
#include <QString>

#include <memory>

namespace Ui {
class Print9030Dialog;
}

class IPrintController;
enum class PrintUiState;

class Print9030Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Print9030Dialog(const QString& projectRoot, QWidget* parent = nullptr);
    Print9030Dialog(const QString& projectRoot, IPrintController* controller,
        QWidget* parent = nullptr);
    ~Print9030Dialog() override;

    void setElementalMemoryResult(std::shared_ptr<const ElementalMemoryResult> result);
    void setManualImageFolder(const QString& folderPath);
    void applyState(PrintUiState state);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void loadConfig();
    void saveConfigFromUi();
    void populateUiFromConfig();
    void updateConfigFromUi();
    void updateSourceSummary();
    void showPreview();
    void browseFolder();
    void startPrint();
    void requestClose();
    QString configPath() const;

    PrintAxisConfig readAxisFromUi(int row) const;
    void writeAxisToUi(int row, const PrintAxisConfig& axis);

    QString projectRoot_;
    std::unique_ptr<Ui::Print9030Dialog> ui_;
    Print9030Config config_;
    PrintImageSet memoryImages_;
    PrintImageSet folderImages_;
    IPrintController* controller_ = nullptr;
    bool ownsController_ = false;
    bool pendingClose_ = false;
    PrintUiState* stateStorage_ = nullptr;
};

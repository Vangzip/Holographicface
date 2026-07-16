#include "SaveSettingsDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QPushButton>
#include <QToolButton>

#include <cstdlib>
#include <iostream>

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        std::exit(1);
    }
}

ResultSaveSettings meshAndElemental()
{
    ResultSaveSettings settings;
    settings.mesh = true;
    settings.elemental = true;
    return settings;
}

void expectAllDisabled(const ResultSaveSettings& settings, const char* message)
{
    expect(!settings.mesh && !settings.multiview && !settings.elemental, message);
}

void testDefaultsAndIndependentSelections()
{
    SaveSettingsDialog dialog;
    expectAllDisabled(dialog.saveSettings(), "save settings must default off");

    dialog.setSaveSettings(meshAndElemental());
    const ResultSaveSettings selected = dialog.saveSettings();
    expect(selected.mesh && !selected.multiview && selected.elemental,
        "dialog did not preserve an independent selection combination");
}

void testConfirmPreservesSelection()
{
    SaveSettingsDialog dialog;
    dialog.setSaveSettings(meshAndElemental());
    QPushButton* confirmButton = dialog.findChild<QPushButton*>("confirmButton");
    expect(confirmButton != nullptr, "confirm button was not found");

    confirmButton->click();

    expect(dialog.result() == QDialog::Accepted, "confirm must accept the dialog");
    const ResultSaveSettings selected = dialog.saveSettings();
    expect(selected.mesh && !selected.multiview && selected.elemental,
        "confirm must preserve the selected combination");
}

void testCancelClearsSelection()
{
    SaveSettingsDialog dialog;
    dialog.setSaveSettings(meshAndElemental());
    QPushButton* cancelButton = dialog.findChild<QPushButton*>("cancelButton");
    expect(cancelButton != nullptr, "cancel button was not found");

    cancelButton->click();

    expect(dialog.result() == QDialog::Rejected, "cancel must reject the dialog");
    expectAllDisabled(dialog.saveSettings(), "cancel must clear all settings");
}

void testCustomCloseClearsSelection()
{
    SaveSettingsDialog dialog;
    dialog.setSaveSettings(meshAndElemental());
    QToolButton* closeButton = dialog.findChild<QToolButton*>("closeButton");
    expect(closeButton != nullptr, "custom close button was not found");

    closeButton->click();

    expect(dialog.result() == QDialog::Rejected, "custom close must reject the dialog");
    expectAllDisabled(dialog.saveSettings(), "custom close must clear all settings");
}

void testSystemCloseClearsSelection()
{
    SaveSettingsDialog dialog;
    dialog.setSaveSettings(meshAndElemental());
    dialog.show();
    QApplication::processEvents();

    dialog.close();
    QApplication::processEvents();

    expect(dialog.result() == QDialog::Rejected, "system close must reject the dialog");
    expectAllDisabled(dialog.saveSettings(), "system close must clear all settings");
}

void testDirectRejectClearsSelection()
{
    SaveSettingsDialog dialog;
    dialog.setSaveSettings(meshAndElemental());

    dialog.reject();

    expectAllDisabled(dialog.saveSettings(), "reject must clear all settings");
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    testDefaultsAndIndependentSelections();
    testConfirmPreservesSelection();
    testCancelClearsSelection();
    testCustomCloseClearsSelection();
    testSystemCloseClearsSelection();
    testDirectRejectClearsSelection();
    std::cout << "save settings dialog tests passed" << std::endl;
    return 0;
}

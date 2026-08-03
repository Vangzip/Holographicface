#include "InputSettingsDialog.h"
#include "SaveSettingsDialog.h"
#include "NativeUiStyle.h"

#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QTemporaryDir>
#include <QToolButton>

#include <cstdlib>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

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

void testNativeApplicationStyle(QApplication& application)
{
    applyNativeWindowsUiStyle(application);
    expect(application.font().family() == "Microsoft YaHei UI",
        "application font must use Microsoft YaHei UI");
    expect(application.font().pointSize() == 9,
        "application font must use the fixed desktop point size");
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

void testUsesNativeDialogControls()
{
    SaveSettingsDialog dialog;
    expect(!dialog.windowFlags().testFlag(Qt::FramelessWindowHint),
        "save settings must use native window chrome");
    expect(dialog.findChild<QToolButton*>("closeButton") == nullptr,
        "save settings must not contain a custom close button");
    expect(dialog.findChild<QDialogButtonBox*>("buttonBox") != nullptr,
        "save settings must use a standard dialog button box");
    expect(dialog.findChild<QCheckBox*>("meshCheckBox")->style() == QApplication::style(),
        "save settings must use the application checkbox style");
}

void writeFixtureFile(const QString& path, const QByteArray& contents = QByteArray("fixture"))
{
    QFile file(path);
    expect(file.open(QIODevice::WriteOnly | QIODevice::Truncate),
        "input settings fixture could not be created");
    expect(file.write(contents) == contents.size(),
        "input settings fixture could not be written");
}

void writeFixtureImage(const QString& path)
{
    const cv::Mat image(4, 4, CV_8UC3, cv::Scalar(20, 80, 220));
    expect(cv::imwrite(path.toStdString(), image),
        "input settings image fixture could not be written");
}

void expectInputRowsEmpty(const InputSettingsDialog& dialog, const char* message)
{
    const QLineEdit* rgbDepth = dialog.findChild<QLineEdit*>("rgbDepthPathEdit");
    const QLineEdit* mesh = dialog.findChild<QLineEdit*>("meshPathEdit");
    const QLineEdit* multiview = dialog.findChild<QLineEdit*>("multiviewPathEdit");
    expect(rgbDepth && mesh && multiview, "input settings path rows were not found");
    expect(rgbDepth->text().isEmpty() && mesh->text().isEmpty()
            && multiview->text().isEmpty(), message);
    expect(dialog.inputSettings().mode == PipelineInputMode::Camera,
        "empty input settings must restore camera mode");
}

void testInputSettingsMutualExclusion()
{
    InputSettingsDialog dialog;
    dialog.setDirectory(PipelineInputMode::RgbDepth, "C:/fixtures/rgb_depth");
    expect(!dialog.findChild<QLineEdit*>("rgbDepthPathEdit")->text().isEmpty(),
        "RGB/depth path must be shown");

    dialog.setDirectory(PipelineInputMode::Mesh, "C:/fixtures/mesh");
    expect(dialog.findChild<QLineEdit*>("rgbDepthPathEdit")->text().isEmpty(),
        "mesh selection must clear RGB/depth");
    expect(!dialog.findChild<QLineEdit*>("meshPathEdit")->text().isEmpty(),
        "mesh path must be shown");
    expect(dialog.inputSettings().mode == PipelineInputMode::Mesh,
        "mesh row must become active");

    dialog.setDirectory(PipelineInputMode::Multiview, "C:/fixtures/multiview");
    expect(dialog.findChild<QLineEdit*>("meshPathEdit")->text().isEmpty(),
        "multiview selection must clear mesh");
    expect(dialog.inputSettings().mode == PipelineInputMode::Multiview,
        "multiview row must become active");
}

void testInputSettingsUsesNativeControls()
{
    InputSettingsDialog dialog;
    expect(!dialog.windowFlags().testFlag(Qt::FramelessWindowHint),
        "input settings must use native window chrome");
    expect(dialog.findChild<QToolButton*>("closeButton") == nullptr,
        "input settings must not contain a custom close button");
    expect(dialog.findChild<QDialogButtonBox*>("buttonBox") != nullptr,
        "input settings must use a standard dialog button box");
    expect(dialog.findChild<QLineEdit*>("rgbDepthPathEdit")->isReadOnly(),
        "RGB/depth path must be read-only");
    expect(dialog.findChild<QLineEdit*>("meshPathEdit")->isReadOnly(),
        "mesh path must be read-only");
    expect(dialog.findChild<QLineEdit*>("multiviewPathEdit")->isReadOnly(),
        "multiview path must be read-only");
}

void testInputSettingsConfirmPreservesSelection()
{
    QTemporaryDir directory;
    expect(directory.isValid(), "input settings temp directory is invalid");
    writeFixtureFile(QDir(directory.path()).filePath("face_mesh.ply"));
    writeFixtureImage(QDir(directory.path()).filePath("face.jpg"));

    InputSettingsDialog dialog;
    dialog.setDirectory(PipelineInputMode::Mesh, directory.path());
    PipelineInputFiles resolvedFiles;
    std::string validationError;
    if (!resolvePipelineInput(
            dialog.inputSettings(), MultiviewInputSpec{},
            &resolvedFiles, &validationError)) {
        std::cerr << "input settings fixture validation failed: "
                  << validationError << std::endl;
        std::exit(1);
    }
    QDialogButtonBox* buttonBox = dialog.findChild<QDialogButtonBox*>("buttonBox");
    expect(buttonBox != nullptr, "input settings button box was not found");
    buttonBox->button(QDialogButtonBox::Ok)->click();

    expect(dialog.result() == QDialog::Accepted,
        "input settings confirm must accept a valid selection");
    expect(dialog.inputSettings().mode == PipelineInputMode::Mesh,
        "input settings confirm must preserve the selected mode");
}

void testInputSettingsCancelAndCloseClearSelection()
{
    InputSettingsDialog cancelDialog;
    cancelDialog.setDirectory(PipelineInputMode::Mesh, "C:/fixtures/mesh");
    cancelDialog.findChild<QDialogButtonBox*>("buttonBox")
        ->button(QDialogButtonBox::Cancel)->click();
    expect(cancelDialog.result() == QDialog::Rejected,
        "input settings cancel must reject the dialog");
    expectInputRowsEmpty(cancelDialog, "input settings cancel must clear all rows");

    InputSettingsDialog closeDialog;
    closeDialog.setDirectory(PipelineInputMode::RgbDepth, "C:/fixtures/rgb_depth");
    closeDialog.show();
    QApplication::processEvents();
    closeDialog.close();
    QApplication::processEvents();
    expectInputRowsEmpty(closeDialog, "input settings close must clear all rows");

    InputSettingsDialog escapeDialog;
    escapeDialog.setDirectory(PipelineInputMode::Multiview, "C:/fixtures/multiview");
    escapeDialog.show();
    QApplication::processEvents();
    QKeyEvent escapePress(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(&escapeDialog, &escapePress);
    QApplication::processEvents();
    expectInputRowsEmpty(escapeDialog, "input settings Escape must clear all rows");
}

void testConfirmPreservesSelection()
{
    SaveSettingsDialog dialog;
    dialog.setSaveSettings(meshAndElemental());
    QDialogButtonBox* buttonBox = dialog.findChild<QDialogButtonBox*>("buttonBox");
    expect(buttonBox != nullptr, "dialog button box was not found");
    QPushButton* confirmButton = buttonBox->button(QDialogButtonBox::Ok);
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
    QDialogButtonBox* buttonBox = dialog.findChild<QDialogButtonBox*>("buttonBox");
    expect(buttonBox != nullptr, "dialog button box was not found");
    QPushButton* cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    expect(cancelButton != nullptr, "cancel button was not found");

    cancelButton->click();

    expect(dialog.result() == QDialog::Rejected, "cancel must reject the dialog");
    expectAllDisabled(dialog.saveSettings(), "cancel must clear all settings");
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

void testEscapeClearsSelection()
{
    SaveSettingsDialog dialog;
    dialog.setSaveSettings(meshAndElemental());
    dialog.show();
    QApplication::processEvents();

    QKeyEvent escapePress(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(&dialog, &escapePress);
    QApplication::processEvents();

    expect(dialog.result() == QDialog::Rejected, "Escape must reject the dialog");
    expectAllDisabled(dialog.saveSettings(), "Escape must clear all settings");
}

void testCaptureWindowUnifiedSettingsButton()
{
    const QDir applicationDirectory(QCoreApplication::applicationDirPath());
    QFile uiFile(applicationDirectory.absoluteFilePath("../../../ui/CaptureWindow.ui"));
    expect(uiFile.open(QIODevice::ReadOnly | QIODevice::Text),
        "CaptureWindow.ui could not be read");
    const QByteArray uiText = uiFile.readAll();
    const int retakeButton = uiText.indexOf("name=\"retakeButton\"");
    const int settingsButton = uiText.indexOf("name=\"settingsButton\"");
    expect(retakeButton >= 0 && settingsButton > retakeButton,
        "unified settings button must follow retake");
    expect(!uiText.contains("name=\"inputSettingsButton\"")
            && !uiText.contains("name=\"printSettingsButton\"")
            && !uiText.contains("name=\"saveSettingsButton\""),
        "legacy settings buttons must be removed");
}

QByteArray readProjectFile(const QString& relativePath, const char* errorMessage)
{
    const QDir applicationDirectory(QCoreApplication::applicationDirPath());
    QFile file(applicationDirectory.absoluteFilePath("../../../" + relativePath));
    expect(file.open(QIODevice::ReadOnly | QIODevice::Text), errorMessage);
    return file.readAll();
}

void testCaptureWindowInputConfigPlaceholders()
{
    const QByteArray templateText = readProjectFile(
        "config/ui_pipeline_template.ini",
        "ui_pipeline_template.ini could not be read for input checks");
    expect(templateText.contains("input_mode={{input_mode}}"),
        "UI config must include the input mode placeholder");
    expect(templateText.contains("input_dir={{input_dir}}"),
        "UI config must include the input directory placeholder");
}

void testFormsUseNativeApplicationStyle()
{
    const QByteArray captureUi = readProjectFile(
        "ui/CaptureWindow.ui", "CaptureWindow.ui could not be read for style checks");
    const QByteArray captureCpp = readProjectFile(
        "widgets/CaptureWindow.cpp", "CaptureWindow.cpp could not be read for style checks");
    const QByteArray saveUi = readProjectFile(
        "ui/SaveSettingsDialog.ui", "SaveSettingsDialog.ui could not be read for style checks");
    const QByteArray printUi = readProjectFile(
        "ui/Print9030Dialog.ui", "Print9030Dialog.ui could not be read for style checks");
    const QByteArray inputUi = readProjectFile(
        "ui/InputSettingsDialog.ui", "InputSettingsDialog.ui could not be read for style checks");

    expect(!captureUi.contains("border: 2px solid #111111"),
        "capture form must not contain thick black borders");
    expect(!captureUi.contains("QProgressBar::chunk"),
        "capture form must use the native progress bar");
    expect(!captureCpp.contains("button->setStyleSheet"),
        "capture buttons must use the application style");
    expect(!saveUi.contains("<property name=\"styleSheet\">"),
        "save settings form must use the native application style");
    expect(saveUi.contains("<widget class=\"QDialogButtonBox\" name=\"buttonBox\">"),
        "save settings form must contain the standard dialog button box");
    expect(!printUi.contains("<property name=\"styleSheet\">"),
        "print form must use the native application style");
    expect(!inputUi.contains("<property name=\"styleSheet\">"),
        "input settings form must use the native application style");
    expect(inputUi.contains("<widget class=\"QDialogButtonBox\" name=\"buttonBox\">"),
        "input settings form must contain the standard dialog button box");
}

void saveInputSettingsSnapshotWhenRequested()
{
    const QString snapshotPath = qEnvironmentVariable("MERGEHOLO_INPUT_UI_SNAPSHOT");
    if (snapshotPath.isEmpty()) {
        return;
    }

    InputSettingsDialog dialog;
    dialog.show();
    QApplication::processEvents();
    expect(dialog.grab().save(snapshotPath),
        "input settings UI snapshot could not be saved");
    dialog.close();
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    testNativeApplicationStyle(application);
    testDefaultsAndIndependentSelections();
    testUsesNativeDialogControls();
    testInputSettingsMutualExclusion();
    testInputSettingsUsesNativeControls();
    testInputSettingsConfirmPreservesSelection();
    testInputSettingsCancelAndCloseClearSelection();
    testConfirmPreservesSelection();
    testCancelClearsSelection();
    testSystemCloseClearsSelection();
    testDirectRejectClearsSelection();
    testEscapeClearsSelection();
    testCaptureWindowUnifiedSettingsButton();
    testCaptureWindowInputConfigPlaceholders();
    testFormsUseNativeApplicationStyle();
    saveInputSettingsSnapshotWhenRequested();
    std::cout << "save settings dialog tests passed" << std::endl;
    return 0;
}

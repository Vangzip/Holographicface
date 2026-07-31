#include "Print9030Dialog.h"
#include "PrintController.h"

#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTemporaryDir>

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

class RecordingPrintController final : public IPrintController
{
public:
    QStringList commands;
    Print9030Config startedConfig;
    PrintAxisConfig lastMoveConfig;

    void connectAndHome() override { commands.append("connectAndHome"); }
    void disconnect() override { commands.append("disconnect"); }
    void moveXNegative(double, const PrintAxisConfig& config) override
    { lastMoveConfig = config; commands.append("moveXNegative"); }
    void moveXPositive(double, const PrintAxisConfig& config) override
    { lastMoveConfig = config; commands.append("moveXPositive"); }
    void moveYNegative(double, const PrintAxisConfig& config) override
    { lastMoveConfig = config; commands.append("moveYNegative"); }
    void moveYPositive(double, const PrintAxisConfig& config) override
    { lastMoveConfig = config; commands.append("moveYPositive"); }
    void stopManualMotion() override { commands.append("stopManualMotion"); }
    void start(const Print9030Config& config, const PrintImageSet&) override
    {
        startedConfig = config;
        commands.append("start");
    }
    void pause() override { commands.append("pause"); }
    void resume() override { commands.append("resume"); }
    void cancel() override { commands.append("cancel"); }

    void publishState(PrintUiState state) { emit stateChanged(state); }
    void publishSafeStopCompleted() { emit safeStopCompleted(); }
};

QPushButton* button(Print9030Dialog& dialog, const char* name)
{
    QPushButton* result = dialog.findChild<QPushButton*>(name);
    expect(result != nullptr, name);
    return result;
}

void testOpeningIsHardwareSilentAndExplicitConnectOwnsStartup()
{
    QTemporaryDir root;
    expect(root.isValid(), "temporary project root must exist");
    RecordingPrintController controller;
    Print9030Dialog dialog(root.path(), &controller);

    expect(controller.commands.isEmpty(),
        "opening the print dialog must issue zero controller commands");
    QPushButton* connect = dialog.findChild<QPushButton*>("connectHomeButton");
    expect(connect != nullptr, "connect/home button must exist");
    connect->click();
    expect(controller.commands == QStringList({"connectAndHome"}),
        "only the explicit connect/home button may initiate startup");
}

void testRetainedButtonsMapOnceAndStatesGateCommands()
{
    QTemporaryDir root;
    RecordingPrintController controller;
    Print9030Dialog dialog(root.path(), &controller);

    expect(!button(dialog, "startButton")->isEnabled(), "start is disabled while disconnected");
    expect(!button(dialog, "xPositiveButton")->isEnabled(), "manual motion is disabled while disconnected");
    controller.publishState(PrintUiState::Ready);
    expect(button(dialog, "xPositiveButton")->isEnabled(), "manual motion is enabled when ready");

    QTableWidget* axes = dialog.findChild<QTableWidget*>("axisTable");
    auto* xSpeed = qobject_cast<QSpinBox*>(axes->cellWidget(0, 2));
    expect(xSpeed != nullptr, "axis speed must use a typed spin box");
    xSpeed->setValue(4321);

    button(dialog, "xNegativeButton")->click();
    button(dialog, "xPositiveButton")->click();
    expect(controller.lastMoveConfig.speedOfMovement == 4321,
        "manual X commands must carry the current typed X profile");
    button(dialog, "yNegativeButton")->click();
    button(dialog, "yPositiveButton")->click();
    button(dialog, "manualStopButton")->click();
    button(dialog, "disconnectButton")->click();
    expect(controller.commands == QStringList({"moveXNegative", "moveXPositive",
        "moveYNegative", "moveYPositive", "stopManualMotion", "disconnect"}),
        "every ready-state motion button must dispatch exactly its named command once");

    controller.commands.clear();
    controller.publishState(PrintUiState::Printing);
    expect(!button(dialog, "xPositiveButton")->isEnabled(), "manual motion is locked while printing");
    expect(button(dialog, "pauseButton")->isEnabled(), "pause is enabled while printing");
    expect(button(dialog, "cancelButton")->isEnabled(), "cancel is enabled while printing");
    button(dialog, "pauseButton")->click();
    button(dialog, "cancelButton")->click();
    expect(controller.commands == QStringList({"pause", "cancel"}),
        "print controls must dispatch exactly once");

    controller.commands.clear();
    controller.publishState(PrintUiState::Paused);
    expect(button(dialog, "resumeButton")->isEnabled(), "resume is enabled only when paused");
    button(dialog, "resumeButton")->click();
    expect(controller.commands == QStringList({"resume"}), "resume must dispatch exactly once");

    const PrintUiState states[] = {PrintUiState::Disconnected, PrintUiState::Connecting,
        PrintUiState::Homing, PrintUiState::Ready, PrintUiState::Printing,
        PrintUiState::Paused, PrintUiState::Stopping, PrintUiState::Fault};
    for (PrintUiState state : states) controller.publishState(state);
}

void testValidSourceAndFieldsRoundTripIntoStart()
{
    QTemporaryDir root;
    expect(root.isValid(), "temporary project root must exist");
    const QString imagePath = QDir(root.path()).filePath("frame.bmp");
    QImage image(2, 2, QImage::Format_RGB32);
    image.fill(Qt::white);
    expect(image.save(imagePath), "fixture image must save");

    RecordingPrintController controller;
    Print9030Dialog dialog(root.path(), &controller);
    dialog.setManualImageFolder(root.path());
    controller.publishState(PrintUiState::Ready);
    expect(button(dialog, "startButton")->isEnabled(),
        "start is enabled only for Ready with a valid source");

    expect(dialog.findChild<QDoubleSpinBox*>("moveAdjustSpin") == nullptr
            && dialog.findChild<QDoubleSpinBox*>("delaySpin") == nullptr
            && dialog.findChild<QDoubleSpinBox*>("exposureSpin") == nullptr,
        "inactive legacy timing fields must not be exposed as functional controls");
    dialog.findChild<QDoubleSpinBox*>("rowSpacingSpin")->setValue(1.125);
    dialog.findChild<QDoubleSpinBox*>("columnSpacingSpin")->setValue(2.250);
    dialog.findChild<QDoubleSpinBox*>("widthScaleSpin")->setValue(1.500);
    dialog.findChild<QDoubleSpinBox*>("heightScaleSpin")->setValue(2.500);
    dialog.findChild<QSpinBox*>("addTempPulseSpin")->setValue(16000);
    dialog.findChild<QSpinBox*>("leadPulseSpin")->setValue(1000);
    dialog.findChild<QSpinBox*>("gridRowsSpin")->setValue(1);
    dialog.findChild<QSpinBox*>("gridColumnsSpin")->setValue(1);
    button(dialog, "previewButton")->click();
    QLabel* preview = dialog.findChild<QLabel*>("previewLabel");
    expect(preview && preview->pixmap() && !preview->pixmap()->isNull(),
        "preview button must display the active first image");
    button(dialog, "startButton")->click();
    expect(controller.commands == QStringList({"start"}), "start must dispatch once");
    expect(controller.startedConfig.main.rowSpacingMm == 1.125
            && controller.startedConfig.main.columnSpacingMm == 2.250
            && controller.startedConfig.main.widthScale == 1.500
            && controller.startedConfig.main.heightScale == 2.500
            && controller.startedConfig.main.addTempPulse == 16000
            && controller.startedConfig.main.leadPulse == 1000,
        "all retained typed main fields must round-trip into the snapshot");
    QString loadError;
    const Print9030Config persisted = loadPrint9030Config(
        QDir(root.path()).filePath("config/print_9030.ini"), &loadError);
    expect(loadError.isEmpty() && persisted.main.gridRows == 1
            && persisted.main.gridColumns == 1,
        "retained fields must persist to and reload from print_9030.ini");

    QTableWidget* axes = dialog.findChild<QTableWidget*>("axisTable");
    expect(axes && axes->rowCount() == 2, "only mapped X/Y axis fields may remain");
    for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < 7; ++column) {
            expect(axes->cellWidget(row, column) != nullptr,
                "every retained axis numeric field must use a typed editor");
        }
        expect(qobject_cast<QCheckBox*>(axes->cellWidget(row, 7)) != nullptr,
            "axis direction must use a checkbox");
    }
}

void testCloseDuringPrintWaitsForSafeStopCompletion()
{
    QTemporaryDir root;
    RecordingPrintController controller;
    Print9030Dialog dialog(root.path(), &controller);
    dialog.show();
    controller.publishState(PrintUiState::Printing);
    dialog.close();
    expect(dialog.isVisible(), "active close must wait for safe stop completion");
    expect(controller.commands == QStringList({"cancel"}),
        "active close must request cancellation exactly once");
    controller.publishSafeStopCompleted();
    expect(!dialog.isVisible(), "safe stop completion must release pending close");
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    testOpeningIsHardwareSilentAndExplicitConnectOwnsStartup();
    testRetainedButtonsMapOnceAndStatesGateCommands();
    testValidSourceAndFieldsRoundTripIntoStart();
    testCloseDuringPrintWaitsForSafeStopCompletion();
    std::cout << "print 9030 dialog tests passed" << std::endl;
    return 0;
}

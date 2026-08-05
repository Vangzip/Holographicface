#include "Print9030Dialog.h"
#include "PrintController.h"

#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QThread>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

#include <cstdlib>
#include <functional>
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
    void setLogicalOrigin() override { commands.append("setLogicalOrigin"); }
    void returnToLogicalOrigin() override { commands.append("returnToLogicalOrigin"); }
    void start(const Print9030Config& config, const PrintImageSet&) override
    {
        startedConfig = config;
        commands.append("start");
    }
    void pause() override { commands.append("pause"); }
    void resume() override { commands.append("resume"); }
    void cancel() override { commands.append("cancel"); }

    void publishState(PrintUiState state) { emit stateChanged(state); }
    void publishError(const QString& detail) { emit errorChanged(detail); }
    void publishPositions(double x, double y) { emit positionsChanged(x, y); }
    void publishSafeStopCompleted() { emit safeStopCompleted(); }
};

QPushButton* button(Print9030Dialog& dialog, const char* name)
{
    QPushButton* result = dialog.findChild<QPushButton*>(name);
    expect(result != nullptr, name);
    return result;
}

bool waitUntil(const std::function<bool()>& predicate, int timeoutMs = 5000)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
        QThread::msleep(1);
    }
    return predicate();
}

QStringList nativeV2FileOrder(const QString& directory)
{
    QStringList files;
#ifdef Q_OS_WIN
    const QString pattern = QDir::toNativeSeparators(
        QDir(directory).absoluteFilePath(QStringLiteral("*.*")));
    WIN32_FIND_DATAW data {};
    HANDLE search = FindFirstFileW(
        reinterpret_cast<LPCWSTR>(pattern.utf16()), &data);
    expect(search != INVALID_HANDLE_VALUE,
        "native folder fixture must enumerate");
    do {
        const QString name = QString::fromWCharArray(data.cFileName);
        if (name != QStringLiteral(".") && name != QStringLiteral("..")
            && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            files.append(QDir(directory).absoluteFilePath(name));
        }
    } while (FindNextFileW(search, &data));
    FindClose(search);
#else
    Q_UNUSED(directory);
#endif
    return files;
}

void testOpeningIsHardwareSilentAndExplicitConnectOwnsStartup()
{
    QTemporaryDir root;
    expect(root.isValid(), "temporary project root must exist");
    RecordingPrintController controller;
    Print9030Dialog dialog(root.path(), &controller);

    expect(controller.commands.isEmpty(),
        "opening the print dialog must issue zero controller commands");
    QLabel* sourceSummary = dialog.findChild<QLabel*>("sourceSummaryLabel");
    expect(sourceSummary
            && sourceSummary->text() == QString::fromUtf8(u8"\u672A\u52A0\u8F7D"),
        "dynamic source summary must preserve exact Chinese Unicode text");
    const QString diagnostic = QString::fromUtf8(
        u8"UNRECOGNIZED_IMC_ERROR: \u672A\u8BC6\u522B\u7684 IMC errorcode.h \u9519\u8BEF");
    controller.publishError(diagnostic);
    QCoreApplication::processEvents();
    QLabel* errorDetail = dialog.findChild<QLabel*>("errorDetailLabel");
    expect(errorDetail && errorDetail->text() == diagnostic,
        "controller diagnostics must reach the UI without an encoding conversion");
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

void testOriginControlsDispatchRenderPositionsAndGateWithState()
{
    QTemporaryDir root;
    RecordingPrintController controller;
    Print9030Dialog dialog(root.path(), &controller);

    controller.publishState(PrintUiState::Ready);
    expect(button(dialog, "setOriginButton")->isEnabled(),
        "set origin is enabled only when ready");
    expect(button(dialog, "returnOriginButton")->isEnabled(),
        "return origin is enabled only when ready");
    button(dialog, "setOriginButton")->click();
    button(dialog, "returnOriginButton")->click();
    expect(controller.commands == QStringList({"setLogicalOrigin", "returnToLogicalOrigin"}),
        "origin controls must dispatch exactly once");

    controller.publishState(PrintUiState::Printing);
    expect(!button(dialog, "setOriginButton")->isEnabled()
            && !button(dialog, "returnOriginButton")->isEnabled(),
        "origin controls must be locked during printing");

    controller.publishPositions(1.234, -5.678);
    QLabel* xLabel = dialog.findChild<QLabel*>("xPositionLabel");
    QLabel* yLabel = dialog.findChild<QLabel*>("yPositionLabel");
    expect(xLabel && xLabel->text() == "X: 1.234 mm"
            && yLabel && yLabel->text() == "Y: -5.678 mm",
        "position signals must render three decimal millimeter values");
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
    expect(waitUntil([&] { return button(dialog, "previewButton")->isEnabled(); }),
        "manual image folder must finish loading");
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

void testFolderLoaderPreservesNativeOrderWithoutGridInference()
{
    QTemporaryDir root;
    expect(root.isValid(), "temporary folder must exist");
    struct Fixture {
        const char* name;
        int red;
    };
    const Fixture fixtures[] = {
        {"002003.bmp", 23}, {"001002.bmp", 12}, {"002001.bmp", 21},
        {"001001.bmp", 11}, {"002002.bmp", 22}, {"001003.bmp", 13}
    };
    for (const Fixture& fixture : fixtures) {
        QImage image(1, 1, QImage::Format_RGB32);
        image.fill(qRgb(fixture.red, 0, 0));
        expect(image.save(QDir(root.path()).filePath(fixture.name)),
            "grid fixture image must save");
    }
    const QStringList expectedPaths = nativeV2FileOrder(root.path());
    expect(expectedPaths.size() == 6,
        "native discovery must find every folder fixture");

    QString error;
    const PrintImageFolderLoadResult result =
        loadPrintImagesFromFolderWithGridInfo(root.path(), &error);
    expect(error.isEmpty() && result.images.isValid(),
        "coordinate-looking folder names must load without special handling");
    expect(!result.hasInferredGrid() && result.gridWarning.isEmpty(),
        "V2 folder loading must not infer a grid or produce a filename warning");
    for (int index = 0; index < expectedPaths.size(); ++index) {
        PrintFrame frame;
        expect(result.images.copyFrame(index, &frame, &error) && frame.isValid(),
            "native-order folder frame must remain decodable");
        const QString expectedName = QFileInfo(expectedPaths.at(index)).fileName();
        int expectedRed = -1;
        for (const Fixture& fixture : fixtures) {
            if (expectedName == QString::fromLatin1(fixture.name)) {
                expectedRed = fixture.red;
                break;
            }
        }
        expect(expectedRed >= 0
                && static_cast<unsigned char>(frame.pixels.at(2)) == expectedRed,
            "folder frames must remain in native discovery order");
    }
}

void testFolderLoaderDoesNotWarnForOrdinaryNames()
{
    QTemporaryDir root;
    expect(root.isValid(), "temporary folder must exist");
    QImage image(1, 1, QImage::Format_RGB32);
    image.fill(Qt::white);
    expect(image.save(QDir(root.path()).filePath("front.bmp")),
        "ordinary fixture image must save");
    expect(image.save(QDir(root.path()).filePath("back.bmp")),
        "ordinary fixture image must save");

    QString error;
    const PrintImageFolderLoadResult result =
        loadPrintImagesFromFolderWithGridInfo(root.path(), &error);
    expect(error.isEmpty() && result.images.isValid(), "ordinary folder must load");
    expect(!result.hasInferredGrid() && result.gridWarning.isEmpty(),
        "ordinary folder names must not infer dimensions or produce a warning");
}

void testManualFolderLoadsAsynchronouslyAndAlignsPrintParameters()
{
    QTemporaryDir root;
    expect(root.isValid(), "temporary folder must exist");
    const char* names[] = {"002003.bmp", "001002.bmp", "002001.bmp",
        "001001.bmp", "002002.bmp", "001003.bmp"};
    for (const char* name : names) {
        QImage image(1, 1, QImage::Format_RGB32);
        image.fill(Qt::white);
        expect(image.save(QDir(root.path()).filePath(QString::fromLatin1(name))),
            "dialog grid fixture image must save");
    }

    RecordingPrintController controller;
    Print9030Dialog dialog(root.path(), &controller);
    dialog.show();
    controller.publishState(PrintUiState::Ready);
    QCoreApplication::processEvents();
    dialog.findChild<QSpinBox*>("gridRowsSpin")->setValue(7);
    dialog.findChild<QSpinBox*>("gridColumnsSpin")->setValue(9);
    dialog.setManualImageFolder(root.path());
    expect(!button(dialog, "startButton")->isEnabled(),
        "Start must stay disabled while a folder is loading");
    expect(!button(dialog, "previewButton")->isEnabled(),
        "Preview must stay disabled while a folder is loading");
    expect(!button(dialog, "browseFolderButton")->isEnabled(),
        "folder selection must stay disabled while a folder is loading");
    expect(waitUntil([&] { return button(dialog, "previewButton")->isEnabled(); }),
        "folder loading must complete through the event loop");

    expect(dialog.findChild<QRadioButton*>("folderSourceRadio")->isChecked(),
        "completed folder load must select the folder source");
    expect(dialog.findChild<QSpinBox*>("gridRowsSpin")->value() == 7,
        "successful V2 folder load must preserve configured row count");
    expect(dialog.findChild<QSpinBox*>("gridColumnsSpin")->value() == 9,
        "successful V2 folder load must preserve configured column count");
    expect(dialog.findChild<QLabel*>("errorDetailLabel")->text().isEmpty(),
        "successful V2 folder load must not show a filename-grid warning");

    auto* rowSpacing = dialog.findChild<QDoubleSpinBox*>("rowSpacingSpin");
    auto* columnSpacing = dialog.findChild<QDoubleSpinBox*>("columnSpacingSpin");
    auto* rows = dialog.findChild<QSpinBox*>("gridRowsSpin");
    auto* columns = dialog.findChild<QSpinBox*>("gridColumnsSpin");
    expect(rowSpacing->geometry().x() < columnSpacing->geometry().x()
            && rowSpacing->geometry().y() == columnSpacing->geometry().y(),
        "spacing editors must share a row");
    expect(rows->geometry().x() < columns->geometry().x()
            && rows->geometry().y() == columns->geometry().y(),
        "grid-count editors must share a row");
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
    testOriginControlsDispatchRenderPositionsAndGateWithState();
    testValidSourceAndFieldsRoundTripIntoStart();
    testFolderLoaderPreservesNativeOrderWithoutGridInference();
    testFolderLoaderDoesNotWarnForOrdinaryNames();
    testManualFolderLoadsAsynchronouslyAndAlignsPrintParameters();
    testCloseDuringPrintWaitsForSafeStopCompletion();
    std::cout << "print 9030 dialog tests passed" << std::endl;
    return 0;
}

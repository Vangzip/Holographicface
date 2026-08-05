#include "ProcessingSettings.h"
#include "ProcessingSettingsDialog.h"
#include "ProcessingSettingsStore.h"
#include "LightFieldCapture.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>

#include <cmath>
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

void writeFile(const QString& path, const QByteArray& contents = QByteArray("fixture"))
{
    QFile file(path);
    expect(file.open(QIODevice::WriteOnly | QIODevice::Truncate),
        "fixture file could not be created");
    expect(file.write(contents) == contents.size(),
        "fixture file could not be written");
}

QString createCameraPreset(QTemporaryDir& directory)
{
    expect(directory.isValid(), "camera preset temp directory is invalid");
    writeFile(QDir(directory.path()).filePath("jp.xml"));
    writeFile(QDir(directory.path()).filePath("param.txt"));
    writeFile(QDir(directory.path()).filePath("camera.cen"));
    return directory.path();
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    expect(file.open(QIODevice::ReadOnly), "fixture file could not be read");
    return file.readAll();
}

QString projectFile(const QString& relativePath)
{
    const QStringList candidates = {
        QDir(QCoreApplication::applicationDirPath()).filePath("../../../" + relativePath),
        QDir::current().filePath("../../../" + relativePath),
        QDir::current().filePath("../../" + relativePath),
        QDir::current().filePath(relativePath)
    };
    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }
    expect(false, "project source fixture could not be located");
    return {};
}

ProcessingSettingsPaths createSettingsFiles(QTemporaryDir& directory)
{
    expect(directory.isValid(), "settings store temp directory is invalid");
    const QString configDirectory = QDir(directory.path()).filePath("config");
    expect(QDir().mkpath(QDir(configDirectory).filePath("084C")),
        "settings config directories could not be created");

    writeFile(QDir(configDirectory).filePath("084C/jp.xml"));
    writeFile(QDir(configDirectory).filePath("084C/param.txt"));
    writeFile(QDir(configDirectory).filePath("084C/camera.cen"));
    writeFile(QDir(configDirectory).filePath("default_pipeline.ini"),
        "# pipeline comment\n"
        "multiview_angle=75\n"
        "multiview_per=4\n"
        "multiview_resolution=180\n"
        "target_rows=120\n"
        "target_cols=130\n"
        "jpg_quality=92\n"
        "vendor_pipeline_key=keep-pipeline\n");
    writeFile(QDir(configDirectory).filePath("depth_to_pointcloud_config.cfg"),
        "# point cloud comment\n"
        "focus=110.5\n"
        "disp=1.25\n"
        "step=0.03\n"
        "vendor_depth_key=keep-depth\n");
    writeFile(QDir(configDirectory).filePath("mesh_config.cfg"),
        "# mesh comment\n"
        "reconstruct=2\n"
        "kSearch=24\n"
        "searchradius=0.02\n"
        "leafsize=0.002\n"
        "vendor_mesh_key=keep-mesh\n");
    writeFile(QDir(configDirectory).filePath("default_camera.ini"),
        "# camera comment\n"
        "camera_config_dir=084C\n"
        "capture_rotation=COUNTERCLOCKWISE_90\n"
        "vendor_camera_key=keep-camera\n");

    return ProcessingSettingsPaths::fromProjectRoot(directory.path());
}

void testProcessingSettingsDefaults()
{
    const ProcessingSettings settings = defaultProcessingSettings(
        "C:/MergeHolo", "C:/MergeHolo/config/084C");

    expect(settings.pipeline.multiviewAngle == 90,
        "default angle must match the current pipeline");
    expect(settings.pipeline.multiviewPer == 3,
        "default samples per degree must match the current pipeline");
    expect(settings.pipeline.multiviewResolution == 150,
        "default view resolution must match the current pipeline");
    expect(settings.pipeline.targetRows == 150 && settings.pipeline.targetCols == 150,
        "default elemental dimensions must match the current pipeline");
    expect(settings.camera.exposureMode == 1,
        "default exposure mode must preserve current capture behavior");
    expect(settings.camera.exposureValue == 15000,
        "default exposure value must preserve current capture behavior");
    expect(std::abs(settings.camera.frameRate - 6.0) < 1e-9,
        "default frame rate must preserve current capture behavior");
    expect(settings.camera.cameraInterface == "571",
        "default camera interface must preserve current capture behavior");
    expect(settings.camera.cameraType == "Indigo",
        "default camera type must preserve current capture behavior");
    expect(settings.camera.rotation == CaptureRotation::Clockwise90,
        "default camera rotation must match the inverted installation");
    expect(viewCountPerAxis(settings) == 270,
        "view count must be angle times samples per degree");
    expect(elementalImageCount(settings) == 22500,
        "elemental count must be rows times columns");
}

void testSubjectSizeConversion()
{
    expect(std::abs(subjectSizeFromDistanceScale(2.0) - 2.0) < 1e-9,
        "distance scale 2 must display as subject size 2");
    expect(std::abs(distanceScaleFromSubjectSize(2.0) - 2.0) < 1e-9,
        "subject size 2 must store as distance scale 2");
    expect(subjectSizeFromDistanceScale(1.0) > subjectSizeFromDistanceScale(2.0),
        "a nearer camera must display a larger subject size");
}

void testValidationAcceptsCurrentDefaultsWithCompletePreset()
{
    QTemporaryDir preset;
    ProcessingSettings settings = defaultProcessingSettings("C:/MergeHolo", createCameraPreset(preset));
    expect(validateProcessingSettings(settings).isEmpty(),
        "current defaults with a complete camera preset must validate");
}

void testValidationRejectsInvalidDimensions()
{
    QTemporaryDir preset;
    ProcessingSettings settings = defaultProcessingSettings("C:/MergeHolo", createCameraPreset(preset));
    settings.pipeline.targetRows = 0;
    expect(!validateProcessingSettings(settings).isEmpty(),
        "zero elemental rows must be rejected");
}

void testValidationRejectsMissingExternalInput()
{
    QTemporaryDir preset;
    ProcessingSettings settings = defaultProcessingSettings("C:/MergeHolo", createCameraPreset(preset));
    settings.input.mode = PipelineInputMode::RgbDepth;
    settings.input.directory = std::filesystem::path("Z:/mergeholo/missing-input");
    expect(!validateProcessingSettings(settings).isEmpty(),
        "a missing external input directory must be rejected");
}

void testValidationRejectsIncompleteCameraPreset()
{
    QTemporaryDir preset;
    expect(preset.isValid(), "incomplete camera preset temp directory is invalid");
    writeFile(QDir(preset.path()).filePath("jp.xml"));
    writeFile(QDir(preset.path()).filePath("param.txt"));

    ProcessingSettings settings = defaultProcessingSettings("C:/MergeHolo", preset.path());
    expect(!validateProcessingSettings(settings).isEmpty(),
        "a camera preset without a cen calibration file must be rejected");
}

void testSettingsStoreLoadsKnownValuesAndCameraFallbacks()
{
    QTemporaryDir directory;
    const ProcessingSettingsPaths paths = createSettingsFiles(directory);
    ProcessingSettings settings = defaultProcessingSettings(
        directory.path(), QDir(directory.path()).filePath("config/084C"));
    QString error;

    expect(loadProcessingSettings(paths, &settings, &error), qPrintable(error));
    expect(settings.pipeline.multiviewAngle == 75, "pipeline angle must load");
    expect(settings.pipeline.multiviewPer == 4, "pipeline sampling must load");
    expect(settings.pointCloud.focus == 110.5, "point-cloud focus must load");
    expect(settings.mesh.kSearch == 24, "mesh neighbor count must load");
    expect(settings.camera.exposureValue == 15000,
        "missing camera exposure must keep the backward-compatible default");
    expect(settings.camera.rotation == CaptureRotation::CounterClockwise90,
        "camera rotation must load");
    expect(QDir::cleanPath(settings.camera.configDirectory)
            == QDir::cleanPath(QDir(directory.path()).filePath("config/084C")),
        "relative camera directory must resolve from the camera ini");
}

void testSettingsStoreUsesSafeCameraRotationFallback()
{
    QTemporaryDir directory;
    const ProcessingSettingsPaths paths = createSettingsFiles(directory);
    QString error;

    writeFile(paths.cameraConfig,
        "camera_config_dir=084C\n");
    ProcessingSettings missing = defaultProcessingSettings(
        directory.path(), QDir(directory.path()).filePath("config/084C"));
    expect(loadProcessingSettings(paths, &missing, &error), qPrintable(error));
    expect(missing.camera.rotation == CaptureRotation::Clockwise90,
        "missing camera rotation must use the inverted-installation default");

    writeFile(paths.cameraConfig,
        "camera_config_dir=084C\n"
        "capture_rotation=sideways\n");
    ProcessingSettings invalid = defaultProcessingSettings(
        directory.path(), QDir(directory.path()).filePath("config/084C"));
    expect(loadProcessingSettings(paths, &invalid, &error), qPrintable(error));
    expect(invalid.camera.rotation == CaptureRotation::Clockwise90,
        "invalid camera rotation must use the safe default");
}

void testSettingsStorePreservesCommentsAndUnknownKeys()
{
    QTemporaryDir directory;
    const ProcessingSettingsPaths paths = createSettingsFiles(directory);
    ProcessingSettings settings = defaultProcessingSettings(
        directory.path(), QDir(directory.path()).filePath("config/084C"));
    QString error;
    expect(loadProcessingSettings(paths, &settings, &error), qPrintable(error));

    settings.pipeline.multiviewAngle = 90;
    settings.pointCloud.focus = 105.0;
    settings.pointCloud.meanK = 64;
    settings.pointCloud.radiusSearch = 0.075;
    settings.mesh.kSearch = 20;
    settings.mesh.maximumNearestNeighbors = 128;
    settings.mesh.textureFocus = 1800.0;
    settings.camera.exposureValue = 12000;
    settings.camera.rotation = CaptureRotation::Clockwise90;
    expect(saveProcessingSettings(paths, settings, &error), qPrintable(error));

    const QByteArray pipeline = readFile(paths.pipelineConfig);
    const QByteArray pointCloud = readFile(paths.pointCloudConfig);
    const QByteArray mesh = readFile(paths.meshConfig);
    const QByteArray camera = readFile(paths.cameraConfig);
    expect(pipeline.contains("# pipeline comment"), "pipeline comment must survive");
    expect(pipeline.contains("vendor_pipeline_key=keep-pipeline"),
        "unknown pipeline key must survive");
    expect(pipeline.contains("multiview_angle=90"), "pipeline key must update");
    expect(pointCloud.contains("vendor_depth_key=keep-depth"),
        "unknown point-cloud key must survive");
    expect(pointCloud.contains("meanK=64") && pointCloud.contains("radiussearch=0.075"),
        "point-cloud detail keys must persist");
    expect(mesh.contains("vendor_mesh_key=keep-mesh"),
        "unknown mesh key must survive");
    expect(mesh.contains("maximumNearestNeighbors=128") && mesh.contains("focus=1800"),
        "mesh detail keys must persist");
    expect(camera.contains("# camera comment"), "camera comment must survive");
    expect(camera.contains("vendor_camera_key=keep-camera"),
        "unknown camera key must survive");
    expect(camera.contains("exposure_value=12000"),
        "new camera exposure key must be appended");
    expect(camera.contains("capture_rotation=clockwise_90"),
        "camera rotation must persist");
}

void testDialogNavigationAndNativeShell()
{
    ProcessingSettingsDialog dialog;
    expect(!dialog.windowFlags().testFlag(Qt::FramelessWindowHint),
        "processing settings must use native window chrome");
    expect(dialog.styleSheet().isEmpty(),
        "processing settings must not install a dialog stylesheet");

    QListWidget* navigation = dialog.findChild<QListWidget*>("settingsNavigation");
    QStackedWidget* pages = dialog.findChild<QStackedWidget*>("settingsPages");
    expect(navigation && navigation->count() == 5,
        "processing settings must expose five priority entries");
    expect(pages && pages->count() == 4,
        "processing settings must contain four editable pages");
    expect(navigation->item(0)->text().contains("P0")
            && navigation->item(0)->text().contains(QString::fromUtf8("常用")),
        "first navigation item must be P0 common settings");
    expect(navigation->item(4)->text().contains("P3")
            && navigation->item(4)->text().contains(QString::fromUtf8("打印")),
        "last navigation item must be P3 printing");
    QWidget* devicePage = dialog.findChild<QWidget*>("devicePage");
    expect(devicePage != nullptr, "device page must exist");
    expect(devicePage->findChild<QGroupBox*>("printGroup") == nullptr,
        "device page must not embed printing controls");
}

void testPrintNavigationEmitsWithoutChangingPage()
{
    ProcessingSettingsDialog dialog;
    bool requested = false;
    QObject::connect(&dialog, &ProcessingSettingsDialog::printRequested,
        [&requested] { requested = true; });
    dialog.selectPage(1);
    dialog.findChild<QListWidget*>("settingsNavigation")->setCurrentRow(4);
    QApplication::processEvents();
    expect(requested, "P3 navigation must request the existing print dialog");
    expect(dialog.findChild<QStackedWidget*>("settingsPages")->currentIndex() == 1,
        "P3 navigation must leave the editable page selection unchanged");
}

void testCommonPageBindingsAndDerivedSummary()
{
    QTemporaryDir preset;
    ProcessingSettings settings = defaultProcessingSettings("C:/MergeHolo", createCameraPreset(preset));
    settings.saveResults.mesh = true;
    ProcessingSettingsDialog dialog;
    dialog.setSettings(settings);

    QSpinBox* angle = dialog.findChild<QSpinBox*>("multiviewAngleSpin");
    QSpinBox* per = dialog.findChild<QSpinBox*>("multiviewPerSpin");
    QLabel* summary = dialog.findChild<QLabel*>("derivedSummaryLabel");
    expect(angle && per && summary, "P0 output controls must exist");
    angle->setValue(60);
    per->setValue(4);
    QApplication::processEvents();
    expect(summary->text().contains("240") && summary->text().contains("22500"),
        "P0 summary must update derived view and elemental counts");
    expect(dialog.findChild<QCheckBox*>("saveMeshCheck")->isChecked(),
        "P0 save-result binding must preserve the draft");
}

void testImagingPageBindings()
{
    QTemporaryDir preset;
    ProcessingSettings settings = defaultProcessingSettings("C:/MergeHolo", createCameraPreset(preset));
    settings.pipeline.subjectSize = 2.5;
    settings.pipeline.centerX = 0.1;
    settings.pipeline.centerY = -0.2;
    settings.pipeline.centerZ = 0.3;
    settings.pipeline.rotateZDeg = 12.0;
    settings.pipeline.rotateXDeg = -8.0;
    settings.pipeline.jpgQuality = 95;
    settings.pipeline.elementalFlipSourceY = true;
    settings.pipeline.elementalFlipViewRows = false;

    ProcessingSettingsDialog dialog;
    dialog.setSettings(settings);
    QDoubleSpinBox* subjectSize = dialog.findChild<QDoubleSpinBox*>("subjectSizeSpin");
    QComboBox* direction = dialog.findChild<QComboBox*>("elementalDirectionCombo");
    expect(subjectSize && direction, "imaging controls must exist");
    expect(std::abs(subjectSize->value() - 2.5) < 1e-9,
        "subject size must populate");
    expect(direction->currentIndex() == 1,
        "elemental source flip must map to the second direction preset");

    dialog.findChild<QDoubleSpinBox*>("centerXSpin")->setValue(0.4);
    dialog.findChild<QDoubleSpinBox*>("rotateZSpin")->setValue(20.0);
    dialog.findChild<QSpinBox*>("jpgQualitySpin")->setValue(88);
    const ProcessingSettings changed = dialog.settings();
    expect(std::abs(changed.pipeline.centerX - 0.4) < 1e-9,
        "left-right position must collect");
    expect(std::abs(changed.pipeline.rotateZDeg - 20.0) < 1e-9,
        "horizontal rotation must collect");
    expect(changed.pipeline.jpgQuality == 88, "JPEG quality must collect");
}

void testAdvancedPageBindingsAndHardwareAdaptiveState()
{
    QTemporaryDir preset;
    ProcessingSettings settings = defaultProcessingSettings("C:/MergeHolo", createCameraPreset(preset));
    settings.pointCloud.focus = 110.0;
    settings.pointCloud.disp = 1.2;
    settings.pointCloud.step = 0.03;
    settings.mesh.reconstruct = 2;
    settings.mesh.kSearch = 30;
    settings.mesh.searchRadius = 0.02;
    settings.mesh.leafSize = 0.002;
    settings.pipeline.atlasSize = 0;
    settings.pipeline.writerThreads = 0;

    ProcessingSettingsDialog dialog;
    dialog.setSettings(settings);
    QDoubleSpinBox* focus = dialog.findChild<QDoubleSpinBox*>("pointCloudFocusSpin");
    QComboBox* reconstruct = dialog.findChild<QComboBox*>("reconstructCombo");
    expect(focus && reconstruct, "advanced controls must exist");
    expect(std::abs(focus->value() - 110.0) < 1e-9,
        "point-cloud focus must populate");
    expect(reconstruct->currentData().toInt() == 2,
        "mesh algorithm must populate");
    QCheckBox* adaptive = dialog.findChild<QCheckBox*>("hardwareAdaptiveCheck");
    expect(adaptive && adaptive->isChecked(), "zero atlas and threads must select hardware adaptation");
    expect(!dialog.findChild<QSpinBox*>("atlasSizeSpin")->isEnabled()
            && !dialog.findChild<QSpinBox*>("writerThreadsSpin")->isEnabled(),
        "hardware adaptation must disable manual performance controls");
    expect(dialog.findChild<QPushButton*>("pointCloudDetailsButton") != nullptr
            && dialog.findChild<QPushButton*>("meshDetailsButton") != nullptr,
        "advanced groups must expose detailed parameter dialogs");

    adaptive->setChecked(false);
    dialog.findChild<QSpinBox*>("atlasSizeSpin")->setValue(4096);
    dialog.findChild<QSpinBox*>("writerThreadsSpin")->setValue(4);
    dialog.findChild<QSpinBox*>("meshKSearchSpin")->setValue(26);
    const ProcessingSettings changed = dialog.settings();
    expect(changed.pipeline.atlasSize == 4096 && changed.pipeline.writerThreads == 4,
        "manual performance values must collect when adaptation is disabled");
    expect(changed.mesh.kSearch == 26, "mesh neighbor count must collect");
}

void testAdvancedDetailDialogsUpdateDraft()
{
    ProcessingSettingsDialog dialog;
    dialog.setSettings(defaultProcessingSettings("C:/MergeHolo", "C:/MergeHolo/config/084C"));
    QPushButton* pointDetails = dialog.findChild<QPushButton*>("pointCloudDetailsButton");
    QPushButton* meshDetails = dialog.findChild<QPushButton*>("meshDetailsButton");
    expect(pointDetails && meshDetails, "advanced detail buttons must exist");

    QTimer::singleShot(0, [] {
        QDialog* details = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        expect(details != nullptr, "point-cloud detail dialog must open");
        QSpinBox* meanK = details->findChild<QSpinBox*>("pointDetailMeanKSpin");
        expect(meanK != nullptr, "point-cloud detail controls must exist");
        meanK->setValue(73);
        details->accept();
    });
    pointDetails->click();
    expect(dialog.settings().pointCloud.meanK == 73,
        "point-cloud detail dialog must update the shared draft");

    QTimer::singleShot(0, [] {
        QDialog* details = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        expect(details != nullptr, "mesh detail dialog must open");
        QSpinBox* maximumNeighbors = details->findChild<QSpinBox*>(
            "meshDetailMaximumNeighborsSpin");
        expect(maximumNeighbors != nullptr, "mesh detail controls must exist");
        maximumNeighbors->setValue(144);
        details->accept();
    });
    meshDetails->click();
    expect(dialog.settings().mesh.maximumNearestNeighbors == 144,
        "mesh detail dialog must update the shared draft");
}

void testDevicePageBindingsAndBusyState()
{
    QTemporaryDir preset;
    ProcessingSettings settings = defaultProcessingSettings("C:/MergeHolo", createCameraPreset(preset));
    settings.camera.exposureValue = 14000;
    settings.camera.frameRate = 7.5;
    settings.camera.cameraInterface = "571";
    settings.camera.cameraType = "Indigo";
    settings.camera.rotation = CaptureRotation::CounterClockwise90;

    ProcessingSettingsDialog dialog;
    dialog.setSettings(settings);
    QLineEdit* directory = dialog.findChild<QLineEdit*>("cameraConfigDirectoryEdit");
    QSpinBox* exposure = dialog.findChild<QSpinBox*>("cameraExposureValueSpin");
    QDoubleSpinBox* frameRate = dialog.findChild<QDoubleSpinBox*>("cameraFrameRateSpin");
    QLineEdit* cameraInterface = dialog.findChild<QLineEdit*>("cameraInterfaceEdit");
    QComboBox* rotation = dialog.findChild<QComboBox*>("cameraRotationCombo");
    expect(directory && exposure && frameRate && cameraInterface && rotation,
        "device controls must exist");
    expect(directory->isReadOnly() && cameraInterface->isReadOnly(),
        "camera path and identity summary must be read-only");
    expect(exposure->value() == 14000, "camera exposure must populate");
    expect(std::abs(frameRate->value() - 7.5) < 1e-9, "camera frame rate must populate");
    expect(rotation->currentData().toInt()
            == static_cast<int>(CaptureRotation::CounterClockwise90),
        "camera rotation must populate");
    expect(dialog.findChild<QPushButton*>("engineerSettingsButton") != nullptr,
        "device page must expose engineer settings");

    bool cameraTestRequested = false;
    QObject::connect(&dialog, &ProcessingSettingsDialog::cameraTestRequested,
        [&cameraTestRequested](const CameraCaptureSettings&) {
            cameraTestRequested = true;
        });
    dialog.findChild<QPushButton*>("testCameraButton")->click();
    expect(cameraTestRequested,
        "valid camera preset must request a real connection test");

    exposure->setValue(13500);
    rotation->setCurrentIndex(rotation->findData(
        static_cast<int>(CaptureRotation::Clockwise90)));
    const ProcessingSettings changed = dialog.settings();
    expect(changed.camera.exposureValue == 13500, "camera exposure must collect");
    expect(changed.camera.rotation == CaptureRotation::Clockwise90,
        "camera rotation must collect");
    dialog.setBusy(true);
    expect(!dialog.findChild<QWidget*>("devicePage")->isEnabled(),
        "busy state must disable device changes");
}

void testCameraInputUsesTypedSettings()
{
    CameraCaptureSettings settings;
    settings.configDirectory = "C:/MergeHolo/config/084C";
    settings.exposureMode = 1;
    settings.exposureValue = 12345;
    settings.frameRate = 7.0;
    settings.cameraInterface = "571";
    settings.cameraType = "Indigo";
    settings.cameraId = 2;
    settings.gpuId = 1;
    settings.missedFrameThreshold = 77;

    const LightFieldCapture::HoloInData input = makeCameraInput(settings);
    expect(input.iHoloExposeMode == 1 && input.iHoloExposeVal == 12345,
        "camera input must use typed exposure settings");
    expect(std::abs(input.dHoloFrameRate - 7.0) < 1e-9,
        "camera input must use typed frame rate");
    expect(input.strCamSeri == "571" && input.strCamType == "Indigo",
        "camera input must use typed camera identity");
    expect(input.iHoloId == 2 && input.iGpuId == 1 && input.iHoloMissThreshold == 77,
        "camera input must use typed engineer settings");
    expect(!input.bIsReadTeamptureBySerial && input.strSerialPort.empty(),
        "disabled temperature serial settings must retain safe defaults");
}

void testMainWindowUsesUnifiedSettingsEntry()
{
    const QByteArray ui = readFile(projectFile("ui/CaptureWindow.ui"));
    expect(ui.contains("name=\"settingsButton\""),
        "main window must expose the unified settings button");
    expect(!ui.contains("name=\"inputSettingsButton\"")
            && !ui.contains("name=\"printSettingsButton\"")
            && !ui.contains("name=\"saveSettingsButton\""),
        "main window must remove the three legacy settings buttons");

    const QByteArray source = readFile(projectFile("widgets/CaptureWindow.cpp"));
    expect(source.contains("ProcessingSettingsDialog"),
        "main window must open the unified settings dialog");
    expect(source.contains("saveProcessingSettings"),
        "accepted unified settings must be persisted");
    expect(source.contains("makeCameraInput(settings_.camera)"),
        "camera startup must use the typed camera settings");

    const QByteArray pipelineTemplate = readFile(
        projectFile("config/ui_pipeline_template.ini"));
    expect(pipelineTemplate.contains("multiview_angle={{multiview_angle}}")
            && pipelineTemplate.contains("jpg_quality={{jpg_quality}}")
            && pipelineTemplate.contains("elemental_writer_threads={{elemental_writer_threads}}"),
        "runtime pipeline template must expose unified settings placeholders");
}

void testMainWindowInitializesCameraBeforeEventLoop()
{
    const QByteArray source = readFile(projectFile("widgets/CaptureWindow.cpp"));
    const QByteArray entryPoint = readFile(projectFile("apps/mergeholo_main.cpp"));
    expect(!source.contains("QTimer::singleShot(0, this, [this] { startCamera(); });"),
        "the first camera initialization must not be queued after the Qt event loop begins");
    expect(entryPoint.indexOf("capture->initialize(&input)")
            < entryPoint.indexOf("CaptureWindow window"),
        "the camera SDK must initialize before the capture window is constructed");
    expect(source.contains("正在使用外部输入"),
        "external input must report its active mode instead of camera initialization failure");
    expect(source.contains("if (settings_.input.isExternal())"),
        "capture-window startup must branch on external input before reporting camera failure");
    expect(source.contains("restartForCameraMode"),
        "switching to camera mode must relaunch instead of initializing the SDK in the active GUI");
    expect(source.contains("QProcess::startDetached"),
        "camera-mode restart must launch a fresh MergeHolo process");
}

void testCliCaptureUsesUnifiedCameraSettings()
{
    const QByteArray sessionSource = readFile(projectFile("camera/CaptureSession.cpp"));
    expect(sessionSource.contains("makeCameraInput(options.cameraSettings)"),
        "CLI capture must reuse typed camera settings");
    const QByteArray mainSource = readFile(projectFile("apps/mergeholo_main.cpp"));
    expect(mainSource.contains("loadProcessingSettings")
            && mainSource.contains("options.cameraSettings = settings.camera"),
        "CLI capture options must load the persisted camera settings");
}

void renderDialogWhenRequested()
{
    const QString screenshotPath = QString::fromLocal8Bit(
        qgetenv("MERGEHOLO_SETTINGS_SCREENSHOT"));
    if (screenshotPath.isEmpty()) {
        return;
    }
    bool pageOk = false;
    const int requestedPage = QString::fromLocal8Bit(
        qgetenv("MERGEHOLO_SETTINGS_PAGE")).toInt(&pageOk);
    ProcessingSettingsDialog dialog;
    dialog.setSettings(defaultProcessingSettings(
        projectFile("."), projectFile("00-bin/config/084C")));
    dialog.selectPage(pageOk ? requestedPage : 0);
    dialog.show();
    QApplication::processEvents();
    expect(dialog.grab().save(screenshotPath),
        "processing settings screenshot could not be saved");
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    testProcessingSettingsDefaults();
    testSubjectSizeConversion();
    testValidationAcceptsCurrentDefaultsWithCompletePreset();
    testValidationRejectsInvalidDimensions();
    testValidationRejectsMissingExternalInput();
    testValidationRejectsIncompleteCameraPreset();
    testSettingsStoreLoadsKnownValuesAndCameraFallbacks();
    testSettingsStoreUsesSafeCameraRotationFallback();
    testSettingsStorePreservesCommentsAndUnknownKeys();
    testDialogNavigationAndNativeShell();
    testPrintNavigationEmitsWithoutChangingPage();
    testCommonPageBindingsAndDerivedSummary();
    testImagingPageBindings();
    testAdvancedPageBindingsAndHardwareAdaptiveState();
    testAdvancedDetailDialogsUpdateDraft();
    testDevicePageBindingsAndBusyState();
    testCameraInputUsesTypedSettings();
    testMainWindowUsesUnifiedSettingsEntry();
    testMainWindowInitializesCameraBeforeEventLoop();
    testCliCaptureUsesUnifiedCameraSettings();
    renderDialogWhenRequested();
    std::cout << "processing settings tests passed" << std::endl;
    return 0;
}

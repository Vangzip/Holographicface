#include "ProcessingSettings.h"
#include "ProcessingSettingsDialog.h"
#include "ProcessingSettingsStore.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTemporaryDir>

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
    expect(QDir::cleanPath(settings.camera.configDirectory)
            == QDir::cleanPath(QDir(directory.path()).filePath("config/084C")),
        "relative camera directory must resolve from the camera ini");
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
    settings.mesh.kSearch = 20;
    settings.camera.exposureValue = 12000;
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
    expect(mesh.contains("vendor_mesh_key=keep-mesh"),
        "unknown mesh key must survive");
    expect(camera.contains("# camera comment"), "camera comment must survive");
    expect(camera.contains("vendor_camera_key=keep-camera"),
        "unknown camera key must survive");
    expect(camera.contains("exposure_value=12000"),
        "new camera exposure key must be appended");
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
    testSettingsStorePreservesCommentsAndUnknownKeys();
    testDialogNavigationAndNativeShell();
    testPrintNavigationEmitsWithoutChangingPage();
    testCommonPageBindingsAndDerivedSummary();
    std::cout << "processing settings tests passed" << std::endl;
    return 0;
}

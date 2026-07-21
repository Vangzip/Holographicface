#include "ProcessingSettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
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

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    testProcessingSettingsDefaults();
    testSubjectSizeConversion();
    testValidationAcceptsCurrentDefaultsWithCompletePreset();
    testValidationRejectsInvalidDimensions();
    testValidationRejectsMissingExternalInput();
    testValidationRejectsIncompleteCameraPreset();
    std::cout << "processing settings tests passed" << std::endl;
    return 0;
}

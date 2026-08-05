#include "CaptureWindow.h"
#include "CaptureImport.h"
#include "CaptureSession.h"
#include "HoloPipeline.h"
#include "NativeUiStyle.h"
#include "PrintHardwareProfile.h"
#include "ProcessingSettingsStore.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <memory>
#include <string>
#include <vector>

namespace {

QString projectRoot()
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    if (QFileInfo::exists(appDir.filePath("../mergeholo.pro"))) {
        return QDir(appDir.filePath("..")).absolutePath();
    }
    if (QFileInfo::exists(appDir.filePath("mergeholo.pro"))) {
        return appDir.absolutePath();
    }
    return appDir.absolutePath();
}

QString firstExisting(const QStringList& candidates, const QString& fallback)
{
    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }
    return QFileInfo(fallback).absoluteFilePath();
}

QString sha256File(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QStringLiteral("unavailable");
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        return QStringLiteral("unavailable");
    }
    return QString::fromLatin1(hash.result().toHex());
}

void logImc60gStartupDiagnostics()
{
    const QString root = projectRoot();
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString runtimePath = QFileInfo(
        QDir(appDir).filePath("IMC_Library_x64.dll")).absoluteFilePath();
    const QString profilePath = firstExisting({
        QDir(root).filePath("config/imc60g_print.ini"),
        QDir(appDir).filePath("config/imc60g_print.ini")
    }, QDir(appDir).filePath("config/imc60g_print.ini"));
    const QString logDirectory = QDir(root).filePath("runs/latest");
    const QString logPath = QDir(logDirectory).filePath("imc60g_startup.log");

    QString profileError;
    const PrintHardwareProfile profile = loadPrintHardwareProfile(profilePath, &profileError);
    const QString architecture = sizeof(void*) == 8
        ? QStringLiteral("x64") : QStringLiteral("not-x64");
    const QStringList lines = {
        QStringLiteral("IMC60G startup architecture=%1").arg(architecture),
        QStringLiteral("IMC60G runtime path=%1")
            .arg(QDir::toNativeSeparators(runtimePath)),
        QStringLiteral("IMC60G runtime sha256=%1").arg(sha256File(runtimePath)),
        QStringLiteral("IMC60G profile version=%1 card=%2 axis_x=%3 axis_y=%4 home_order=Y,X")
            .arg(profile.version)
            .arg(profile.cardIndex)
            .arg(profile.axisX)
            .arg(profile.axisY),
        QStringLiteral("IMC60G exposure backend=SV660N internal position compare DO1"),
        QStringLiteral("IMC60G startup action=diagnostics-only; card remains closed"),
        QStringLiteral("IMC60G startup log path=%1")
            .arg(QDir::toNativeSeparators(QFileInfo(logPath).absoluteFilePath()))
    };

    for (const QString& line : lines) {
        qInfo().noquote() << line;
    }
    if (!profileError.isEmpty()) {
        qWarning().noquote() << "IMC60G profile error=" + profileError;
    }

    if (!QDir().mkpath(logDirectory)) {
        qWarning().noquote() << "IMC60G startup log directory unavailable="
                            + QDir::toNativeSeparators(logDirectory);
        return;
    }
    QFile logFile(logPath);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning().noquote() << "IMC60G startup log unavailable="
                            + QDir::toNativeSeparators(logPath);
        return;
    }
    QTextStream stream(&logFile);
    stream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << '\n';
    for (const QString& line : lines) {
        stream << line << '\n';
    }
    if (!profileError.isEmpty()) {
        stream << "IMC60G profile error=" << profileError << '\n';
    }
    stream.flush();
}

QString readSimpleIniValue(const QString& path, const QString& key)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';') || line.startsWith('[')) {
            continue;
        }
        const int separator = line.indexOf('=');
        if (separator < 0) {
            continue;
        }

        const QString lineKey = line.left(separator).trimmed();
        if (lineKey.compare(key, Qt::CaseInsensitive) == 0) {
            return line.mid(separator + 1).trimmed();
        }
    }
    return {};
}

QString resolvePathRelativeToFile(const QString& filePath, const QString& value)
{
    if (value.isEmpty()) {
        return {};
    }
    const QFileInfo info(value);
    if (info.isAbsolute()) {
        return info.absoluteFilePath();
    }
    return QFileInfo(QDir(QFileInfo(filePath).absolutePath()).filePath(value)).absoluteFilePath();
}

QString optionValue(const QStringList& args, const QString& name, const QString& fallback)
{
    const int index = args.indexOf(name);
    if (index >= 0 && index + 1 < args.size()) {
        return args.at(index + 1);
    }
    return fallback;
}

int intOption(const QStringList& args, const QString& name, int fallback)
{
    bool ok = false;
    const int value = optionValue(args, name, QString()).toInt(&ok);
    return ok ? value : fallback;
}

bool hasOption(const QStringList& args, const QString& name)
{
    return args.contains(name);
}

QString defaultPipelineConfig()
{
    const QString root = projectRoot();
    const QString appConfig = QDir(QCoreApplication::applicationDirPath()).filePath("config/default_pipeline.ini");
    const QString legacyAppConfig = QDir(QCoreApplication::applicationDirPath()).filePath("config/holo_config.merge.ini");
    return firstExisting({
        QDir(root).filePath("config/default_pipeline.ini"),
        appConfig,
        QDir(root).filePath("config/holo_config.merge.ini"),
        legacyAppConfig
    }, appConfig);
}

QString defaultCameraConfig()
{
    const QString root = projectRoot();
    const QString rootCameraIni = QDir(root).filePath("config/default_camera.ini");
    const QString appCameraIni = QDir(QCoreApplication::applicationDirPath()).filePath("config/default_camera.ini");
    const QString cameraIni = firstExisting({ rootCameraIni, appCameraIni }, rootCameraIni);
    const QString configuredDir = readSimpleIniValue(cameraIni, "camera_config_dir");
    if (!configuredDir.isEmpty()) {
        const QString resolved = resolvePathRelativeToFile(cameraIni, configuredDir);
        if (QFileInfo::exists(resolved)) {
            return QFileInfo(resolved).absoluteFilePath();
        }
    }

    const QString appConfig = QDir(QCoreApplication::applicationDirPath()).filePath("config/084C");
    return firstExisting({
        appConfig,
        QDir(root).filePath("runtime/holoLib/config/084C"),
        QDir(root).filePath("../holocamera/00-bin/config/084C")
    }, appConfig);
}

QString defaultCaptureRoot()
{
    return QDir(projectRoot()).filePath("runs/latest/capture");
}

QString defaultPipelineInput()
{
    return QDir(projectRoot()).filePath("runs/latest/pipeline_input");
}

void printUsage()
{
    QTextStream out(stdout);
    out << "MergeHolo\n"
        << "Usage:\n"
        << "  mergeholo.exe [--ui] [--camera-config dir]\n"
        << "  mergeholo.exe --capture [--save-dir dir] [--camera-config dir] [--max-frames n] [--duration seconds] [--no-preview]\n"
        << "  mergeholo.exe --import-capture [--capture-dir dir] [--pipeline-input dir]\n"
        << "  mergeholo.exe --capture-and-run [capture options] [--config ini] [--stage all|depth|mesh|model|multiview|elemental]\n"
        << "  mergeholo.exe --pipeline --config ini [Holo pipeline args]\n"
        << "  mergeholo.exe --config ini [Holo pipeline args]\n";
}

int runPipelineWithArgs(char* exePath, const std::vector<std::string>& args)
{
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    argv.push_back(exePath);
    for (const std::string& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    return runHoloPipelineCli(static_cast<int>(argv.size()), argv.data());
}

int runPipelinePassthrough(int argc, char* argv[], int skipArgs)
{
    std::vector<char*> pipelineArgv;
    pipelineArgv.reserve(static_cast<size_t>(argc - skipArgs + 1));
    pipelineArgv.push_back(argv[0]);
    for (int i = skipArgs + 1; i < argc; ++i) {
        pipelineArgv.push_back(argv[i]);
    }
    return runHoloPipelineCli(static_cast<int>(pipelineArgv.size()), pipelineArgv.data());
}

CaptureSessionOptions captureOptionsFromArgs(const QStringList& args)
{
    CaptureSessionOptions options;
    options.saveRoot = optionValue(args, "--save-dir", defaultCaptureRoot());
    ProcessingSettings settings = defaultProcessingSettings(projectRoot(), defaultCameraConfig());
    QString settingsError;
    if (!loadProcessingSettings(
            ProcessingSettingsPaths::fromProjectRoot(projectRoot()), &settings, &settingsError)) {
        qWarning() << "Cannot load unified camera settings:" << settingsError;
    }
    options.cameraSettings = settings.camera;
    options.cameraSettings.configDirectory = optionValue(
        args, "--camera-config", options.cameraSettings.configDirectory);
    options.minFreeSpaceGb = intOption(args, "--min-free-gb", options.minFreeSpaceGb);
    options.saveIntervalMs = intOption(args, "--save-interval-ms", options.saveIntervalMs);
    options.maxFrames = intOption(args, "--max-frames", options.maxFrames);
    options.durationSeconds = intOption(args, "--duration", options.durationSeconds);
    options.showPreview = !hasOption(args, "--no-preview");
    return options;
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    applyNativeWindowsUiStyle(app);
    logImc60gStartupDiagnostics();
    const QStringList args = app.arguments();

    if (hasOption(args, "--mergeholo-help")) {
        printUsage();
        return 0;
    }

    if (args.size() <= 1 || args.at(1) == "--ui") {
        const QString root = projectRoot();
        const QString cameraConfig = optionValue(args, "--camera-config", defaultCameraConfig());
        ProcessingSettings settings = defaultProcessingSettings(root, cameraConfig);
        QString settingsError;
        if (!loadProcessingSettings(
                ProcessingSettingsPaths::fromProjectRoot(root), &settings, &settingsError)) {
            qWarning() << "Cannot load unified camera settings:" << settingsError;
        }

        // The vendor SDK must open the GigE device before the first QWidget is
        // constructed. Initializing it after a native window exists can block
        // forever even though the same device works in the vendor application.
        std::unique_ptr<LightFieldCapture> capture;
        if (!settings.input.isExternal()) {
            capture = std::make_unique<LightFieldCapture>();
            const LightFieldCapture::HoloInData input = makeCameraInput(settings.camera);
            if (!capture->initialize(&input)) {
                capture.reset();
                qWarning() << "Camera initialization failed before UI startup.";
            }
        }

        CaptureWindow window(root, cameraConfig, std::move(capture));
        window.show();
        return app.exec();
    }

    const QString mode = args.at(1);
    if (mode == "--capture") {
        return runCaptureSession(captureOptionsFromArgs(args));
    }

    if (mode == "--import-capture") {
        CaptureImportOptions options;
        options.captureRoot = optionValue(args, "--capture-dir", defaultCaptureRoot());
        options.pipelineInputDir = optionValue(args, "--pipeline-input", defaultPipelineInput());
        options.overwrite = !hasOption(args, "--no-overwrite");
        return importCaptureForPipeline(options);
    }

    if (mode == "--capture-and-run") {
        const CaptureSessionOptions captureOptions = captureOptionsFromArgs(args);
        int code = runCaptureSession(captureOptions);
        if (code != 0) {
            return code;
        }

        CaptureImportOptions importOptions;
        importOptions.captureRoot = captureOptions.saveRoot;
        importOptions.pipelineInputDir = optionValue(args, "--pipeline-input", defaultPipelineInput());
        importOptions.overwrite = !hasOption(args, "--no-overwrite");
        code = importCaptureForPipeline(importOptions);
        if (code != 0) {
            return code;
        }

        const QString config = optionValue(args, "--config", defaultPipelineConfig());
        const QString stage = optionValue(args, "--stage", "all");
        return runPipelineWithArgs(argv[0], {
            "--config", config.toStdString(),
            "--stage", stage.toStdString()
        });
    }

    if (mode == "--pipeline") {
        return runPipelinePassthrough(argc, argv, 1);
    }

    return runHoloPipelineCli(argc, argv);
}

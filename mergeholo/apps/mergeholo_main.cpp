#include "CaptureImport.h"
#include "CaptureSession.h"
#include "HoloPipeline.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTextStream>

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
    const QString appConfig = QDir(QCoreApplication::applicationDirPath()).filePath("config/holo_config.merge.ini");
    return firstExisting({ QDir(root).filePath("config/holo_config.merge.ini"), appConfig }, appConfig);
}

QString defaultCameraConfig()
{
    const QString root = projectRoot();
    const QString appConfig = QDir(QCoreApplication::applicationDirPath()).filePath("config/holoConf-023C");
    return firstExisting({
        appConfig,
        QDir(root).filePath("runtime/holoLib/config/holoConf-023C"),
        QDir(root).filePath("../holocamera/00-bin/config/holoConf-023C")
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
    options.cameraConfigPath = optionValue(args, "--camera-config", defaultCameraConfig());
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
    const QStringList args = app.arguments();

    if (args.size() <= 1 || hasOption(args, "--mergeholo-help")) {
        printUsage();
        return args.size() <= 1 ? 0 : 0;
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

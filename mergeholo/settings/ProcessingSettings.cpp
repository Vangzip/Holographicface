#include "ProcessingSettings.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include <algorithm>

namespace {

QString displayPath(const std::filesystem::path& path)
{
#ifdef _WIN32
    return QString::fromStdWString(path.wstring());
#else
    return QString::fromStdString(path.string());
#endif
}

bool hasCalibrationFile(const QDir& directory)
{
    return !directory.entryList({ "*.cen" }, QDir::Files).isEmpty();
}

} // namespace

ProcessingSettings defaultProcessingSettings(
    const QString& projectRoot,
    const QString& cameraConfigDirectory)
{
    ProcessingSettings settings;
    settings.pipeline.outputRoot = QDir(projectRoot).absoluteFilePath("output");
    settings.camera.configDirectory = QDir(cameraConfigDirectory).absolutePath();
    return settings;
}

double subjectSizeFromDistanceScale(double distanceScale)
{
    return distanceScale > 0.0 ? 4.0 / distanceScale : 0.0;
}

double distanceScaleFromSubjectSize(double subjectSize)
{
    return subjectSize > 0.0 ? 4.0 / subjectSize : 0.0;
}

int viewCountPerAxis(const ProcessingSettings& settings)
{
    return settings.pipeline.multiviewAngle * settings.pipeline.multiviewPer;
}

qint64 elementalImageCount(const ProcessingSettings& settings)
{
    return static_cast<qint64>(settings.pipeline.targetRows)
        * static_cast<qint64>(settings.pipeline.targetCols);
}

QString validateProcessingSettings(const ProcessingSettings& settings)
{
    const PipelineUiSettings& pipeline = settings.pipeline;
    if (pipeline.multiviewAngle < 1 || pipeline.multiviewAngle > 360) {
        return QString::fromUtf8("视角范围必须在 1 到 360 度之间。");
    }
    if (pipeline.multiviewPer < 1 || pipeline.multiviewPer > 10) {
        return QString::fromUtf8("每度采样数必须在 1 到 10 之间。");
    }
    if (pipeline.multiviewResolution < 16 || pipeline.multiviewResolution > 4096) {
        return QString::fromUtf8("单视图分辨率必须在 16 到 4096 之间。");
    }
    if (pipeline.targetRows < 1 || pipeline.targetRows > 4096
        || pipeline.targetCols < 1 || pipeline.targetCols > 4096) {
        return QString::fromUtf8("Elemental 行列必须在 1 到 4096 之间。");
    }
    if (pipeline.subjectSize < 0.5 || pipeline.subjectSize > 5.0) {
        return QString::fromUtf8("人物大小必须在 0.5 到 5.0 之间。");
    }
    if (pipeline.jpgQuality < 1 || pipeline.jpgQuality > 100) {
        return QString::fromUtf8("JPEG 质量必须在 1 到 100 之间。");
    }

    if (settings.input.isExternal()) {
        const QString inputDirectory = displayPath(settings.input.directory);
        if (inputDirectory.isEmpty() || !QDir(inputDirectory).exists()) {
            return QString::fromUtf8("外部输入目录不存在：\n")
                + QDir::toNativeSeparators(inputDirectory);
        }
    }

    if (pipeline.outputRoot.trimmed().isEmpty()) {
        return QString::fromUtf8("输出目录不能为空。");
    }

    const QDir cameraDirectory(settings.camera.configDirectory);
    if (!cameraDirectory.exists()) {
        return QString::fromUtf8("相机配置目录不存在：\n")
            + QDir::toNativeSeparators(settings.camera.configDirectory);
    }
    if (!QFileInfo(cameraDirectory.filePath("jp.xml")).isFile()
        || !QFileInfo(cameraDirectory.filePath("param.txt")).isFile()
        || !hasCalibrationFile(cameraDirectory)) {
        return QString::fromUtf8(
            "相机配置目录必须包含 jp.xml、param.txt 和至少一个 .cen 标定文件。");
    }

    if (settings.camera.exposureValue < 0) {
        return QString::fromUtf8("曝光值不能小于 0。");
    }
    if (settings.camera.frameRate <= 0.0) {
        return QString::fromUtf8("帧率必须大于 0。");
    }

    return {};
}

LightFieldCapture::HoloInData makeCameraInput(const CameraCaptureSettings& settings)
{
    LightFieldCapture::HoloInData input;
    input.iHoloExposeMode = settings.exposureMode;
    input.iHoloExposeVal = settings.exposureValue;
    input.iHoloId = settings.cameraId;
    input.dHoloFrameRate = settings.frameRate;
    input.iHoloMissThreshold = settings.missedFrameThreshold;
    input.bIsReadTeamptureBySerial = false;
    input.strSerialPort.clear();
    input.iSerialBaudRate = 9600;
    input.iSerialDataBits = 8;
    input.iSerialStopBits = 1;
    input.iSerialParity = 0;
    input.strParseCfgPath = QDir::fromNativeSeparators(settings.configDirectory).toStdString();
    input.iGpuId = settings.gpuId;
    input.strCamSeri = settings.cameraInterface.toStdString();
    input.strCamType = settings.cameraType.toStdString();
    return input;
}

#include "ProcessingSettingsStore.h"

#include "KeyValueConfig.h"

#include <QDir>
#include <QFileInfo>

#include <filesystem>

namespace {

int intValue(const KeyValueConfig& config, const QString& key, int fallback)
{
    bool ok = false;
    const int parsed = config.value(key).toInt(&ok);
    return ok ? parsed : fallback;
}

double doubleValue(const KeyValueConfig& config, const QString& key, double fallback)
{
    bool ok = false;
    const double parsed = config.value(key).toDouble(&ok);
    return ok ? parsed : fallback;
}

bool boolValue(const KeyValueConfig& config, const QString& key, bool fallback)
{
    const QString value = config.value(key).trimmed().toLower();
    if (value == "1" || value == "true" || value == "yes" || value == "on") {
        return true;
    }
    if (value == "0" || value == "false" || value == "no" || value == "off") {
        return false;
    }
    return fallback;
}

QString boolText(bool value)
{
    return value ? "true" : "false";
}

QString resolvedPath(const QString& configPath, const QString& value)
{
    if (value.trimmed().isEmpty()) {
        return {};
    }
    QFileInfo valueInfo(QDir::fromNativeSeparators(value));
    if (valueInfo.isAbsolute()) {
        return QDir::cleanPath(valueInfo.absoluteFilePath());
    }
    return QDir::cleanPath(QDir(QFileInfo(configPath).absolutePath()).absoluteFilePath(value));
}

QString storedPath(const QString& configPath, const QString& value)
{
    if (value.trimmed().isEmpty()) {
        return {};
    }
    const QDir configDirectory(QFileInfo(configPath).absolutePath());
    return QDir::fromNativeSeparators(configDirectory.relativeFilePath(QFileInfo(value).absoluteFilePath()));
}

std::filesystem::path filesystemPath(const QString& path)
{
#ifdef _WIN32
    return std::filesystem::path(path.toStdWString()).lexically_normal();
#else
    return std::filesystem::path(path.toStdString()).lexically_normal();
#endif
}

bool loadAll(
    const ProcessingSettingsPaths& paths,
    KeyValueConfig* pipeline,
    KeyValueConfig* pointCloud,
    KeyValueConfig* mesh,
    KeyValueConfig* camera,
    QString* errorMessage)
{
    return pipeline->load(paths.pipelineConfig, errorMessage)
        && pointCloud->load(paths.pointCloudConfig, errorMessage)
        && mesh->load(paths.meshConfig, errorMessage)
        && camera->load(paths.cameraConfig, errorMessage);
}

} // namespace

ProcessingSettingsPaths ProcessingSettingsPaths::fromProjectRoot(const QString& projectRoot)
{
    ProcessingSettingsPaths paths;
    paths.projectRoot = QDir(projectRoot).absolutePath();
    const QDir configDirectory(QDir(paths.projectRoot).filePath("config"));
    paths.pipelineConfig = configDirectory.filePath("default_pipeline.ini");
    paths.pointCloudConfig = configDirectory.filePath("depth_to_pointcloud_config.cfg");
    paths.meshConfig = configDirectory.filePath("mesh_config.cfg");
    paths.cameraConfig = configDirectory.filePath("default_camera.ini");
    return paths;
}

bool loadProcessingSettings(
    const ProcessingSettingsPaths& paths,
    ProcessingSettings* settings,
    QString* errorMessage)
{
    if (!settings) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8("设置输出对象不能为空。");
        }
        return false;
    }

    KeyValueConfig pipeline;
    KeyValueConfig pointCloud;
    KeyValueConfig mesh;
    KeyValueConfig camera;
    if (!loadAll(paths, &pipeline, &pointCloud, &mesh, &camera, errorMessage)) {
        return false;
    }

    PipelineInputMode inputMode = settings->input.mode;
    if (parsePipelineInputMode(pipeline.value("input_mode").toStdString(), &inputMode)) {
        settings->input.mode = inputMode;
    }
    const QString inputDirectory = resolvedPath(paths.pipelineConfig, pipeline.value("input_dir"));
    settings->input.directory = inputDirectory.isEmpty()
        ? std::filesystem::path{} : filesystemPath(inputDirectory);

    PipelineUiSettings& pipelineSettings = settings->pipeline;
    const QString outputRoot = resolvedPath(paths.pipelineConfig, pipeline.value("output_root"));
    if (!outputRoot.isEmpty()) pipelineSettings.outputRoot = outputRoot;
    pipelineSettings.multiviewAngle = intValue(pipeline, "multiview_angle", pipelineSettings.multiviewAngle);
    pipelineSettings.multiviewPer = intValue(pipeline, "multiview_per", pipelineSettings.multiviewPer);
    pipelineSettings.multiviewResolution = intValue(pipeline, "multiview_resolution", pipelineSettings.multiviewResolution);
    pipelineSettings.targetRows = intValue(pipeline, "target_rows", pipelineSettings.targetRows);
    pipelineSettings.targetCols = intValue(pipeline, "target_cols", pipelineSettings.targetCols);
    pipelineSettings.subjectSize = subjectSizeFromDistanceScale(
        doubleValue(pipeline, "multiview_camera_distance_scale",
            distanceScaleFromSubjectSize(pipelineSettings.subjectSize)));
    pipelineSettings.centerX = doubleValue(pipeline, "multiview_camera_center_offset_x", pipelineSettings.centerX);
    pipelineSettings.centerY = doubleValue(pipeline, "multiview_camera_center_offset_y", pipelineSettings.centerY);
    pipelineSettings.centerZ = doubleValue(pipeline, "multiview_camera_center_offset_z", pipelineSettings.centerZ);
    pipelineSettings.rotateXDeg = doubleValue(pipeline, "multiview_initial_rotate_x_deg", pipelineSettings.rotateXDeg);
    pipelineSettings.rotateZDeg = doubleValue(pipeline, "multiview_initial_rotate_z_deg", pipelineSettings.rotateZDeg);
    pipelineSettings.jpgQuality = intValue(pipeline, "jpg_quality", pipelineSettings.jpgQuality);
    pipelineSettings.captureFlipVertical = boolValue(pipeline, "multiview_capture_flip_vertical", pipelineSettings.captureFlipVertical);
    pipelineSettings.elementalFlipSourceY = boolValue(pipeline, "elemental_flip_source_y", pipelineSettings.elementalFlipSourceY);
    pipelineSettings.elementalFlipViewRows = boolValue(pipeline, "elemental_flip_view_rows", pipelineSettings.elementalFlipViewRows);
    pipelineSettings.atlasSize = intValue(pipeline, "multiview_atlas_size", pipelineSettings.atlasSize);
    pipelineSettings.writerThreads = intValue(pipeline, "elemental_writer_threads", pipelineSettings.writerThreads);
    settings->saveResults.mesh = boolValue(pipeline, "save_mesh_result", settings->saveResults.mesh);
    settings->saveResults.multiview = boolValue(pipeline, "save_multiview_result", settings->saveResults.multiview);
    settings->saveResults.elemental = boolValue(pipeline, "save_elemental_result", settings->saveResults.elemental);

    settings->pointCloud.focus = doubleValue(pointCloud, "focus", settings->pointCloud.focus);
    settings->pointCloud.disp = doubleValue(pointCloud, "disp", settings->pointCloud.disp);
    settings->pointCloud.step = doubleValue(pointCloud, "step", settings->pointCloud.step);
    settings->pointCloud.outlierFilterEnabled = boolValue(
        pointCloud, "outlier_filter_enabled", settings->pointCloud.outlierFilterEnabled);

    settings->mesh.reconstruct = intValue(mesh, "reconstruct", settings->mesh.reconstruct);
    settings->mesh.kSearch = intValue(mesh, "kSearch", settings->mesh.kSearch);
    settings->mesh.searchRadius = doubleValue(mesh, "searchradius", settings->mesh.searchRadius);
    settings->mesh.leafSize = doubleValue(mesh, "leafsize", settings->mesh.leafSize);

    const QString cameraDirectory = resolvedPath(paths.cameraConfig, camera.value("camera_config_dir"));
    if (!cameraDirectory.isEmpty()) settings->camera.configDirectory = cameraDirectory;
    settings->camera.exposureMode = intValue(camera, "exposure_mode", settings->camera.exposureMode);
    settings->camera.exposureValue = intValue(camera, "exposure_value", settings->camera.exposureValue);
    settings->camera.frameRate = doubleValue(camera, "frame_rate", settings->camera.frameRate);
    settings->camera.cameraInterface = camera.value("camera_interface", settings->camera.cameraInterface);
    settings->camera.cameraType = camera.value("camera_type", settings->camera.cameraType);
    settings->camera.cameraId = intValue(camera, "camera_id", settings->camera.cameraId);
    settings->camera.gpuId = intValue(camera, "gpu_id", settings->camera.gpuId);
    settings->camera.missedFrameThreshold = intValue(
        camera, "missed_frame_threshold", settings->camera.missedFrameThreshold);
    return true;
}

bool saveProcessingSettings(
    const ProcessingSettingsPaths& paths,
    const ProcessingSettings& settings,
    QString* errorMessage)
{
    KeyValueConfig pipeline;
    KeyValueConfig pointCloud;
    KeyValueConfig mesh;
    KeyValueConfig camera;
    if (!loadAll(paths, &pipeline, &pointCloud, &mesh, &camera, errorMessage)) {
        return false;
    }

    const PipelineUiSettings& p = settings.pipeline;
    pipeline.setValue("input_mode", QString::fromStdString(pipelineInputModeName(settings.input.mode)));
    pipeline.setValue("input_dir", settings.input.isExternal()
        ? storedPath(paths.pipelineConfig, QString::fromStdWString(settings.input.directory.wstring())) : QString());
    pipeline.setValue("output_root", storedPath(paths.pipelineConfig, p.outputRoot));
    pipeline.setValue("multiview_angle", QString::number(p.multiviewAngle));
    pipeline.setValue("multiview_per", QString::number(p.multiviewPer));
    pipeline.setValue("multiview_resolution", QString::number(p.multiviewResolution));
    pipeline.setValue("target_rows", QString::number(p.targetRows));
    pipeline.setValue("target_cols", QString::number(p.targetCols));
    pipeline.setValue("multiview_camera_distance_scale", QString::number(distanceScaleFromSubjectSize(p.subjectSize), 'g', 15));
    pipeline.setValue("multiview_camera_center_offset_x", QString::number(p.centerX, 'g', 15));
    pipeline.setValue("multiview_camera_center_offset_y", QString::number(p.centerY, 'g', 15));
    pipeline.setValue("multiview_camera_center_offset_z", QString::number(p.centerZ, 'g', 15));
    pipeline.setValue("multiview_initial_rotate_x_deg", QString::number(p.rotateXDeg, 'g', 15));
    pipeline.setValue("multiview_initial_rotate_z_deg", QString::number(p.rotateZDeg, 'g', 15));
    pipeline.setValue("jpg_quality", QString::number(p.jpgQuality));
    pipeline.setValue("multiview_capture_flip_vertical", boolText(p.captureFlipVertical));
    pipeline.setValue("elemental_flip_source_y", boolText(p.elementalFlipSourceY));
    pipeline.setValue("elemental_flip_view_rows", boolText(p.elementalFlipViewRows));
    pipeline.setValue("multiview_atlas_size", QString::number(p.atlasSize));
    pipeline.setValue("elemental_writer_threads", QString::number(p.writerThreads));
    pipeline.setValue("save_mesh_result", boolText(settings.saveResults.mesh));
    pipeline.setValue("save_multiview_result", boolText(settings.saveResults.multiview));
    pipeline.setValue("save_elemental_result", boolText(settings.saveResults.elemental));

    pointCloud.setValue("focus", QString::number(settings.pointCloud.focus, 'g', 15));
    pointCloud.setValue("disp", QString::number(settings.pointCloud.disp, 'g', 15));
    pointCloud.setValue("step", QString::number(settings.pointCloud.step, 'g', 15));
    pointCloud.setValue("outlier_filter_enabled", boolText(settings.pointCloud.outlierFilterEnabled));

    mesh.setValue("reconstruct", QString::number(settings.mesh.reconstruct));
    mesh.setValue("kSearch", QString::number(settings.mesh.kSearch));
    mesh.setValue("searchradius", QString::number(settings.mesh.searchRadius, 'g', 15));
    mesh.setValue("leafsize", QString::number(settings.mesh.leafSize, 'g', 15));

    camera.setValue("camera_config_dir", storedPath(paths.cameraConfig, settings.camera.configDirectory));
    camera.setValue("exposure_mode", QString::number(settings.camera.exposureMode));
    camera.setValue("exposure_value", QString::number(settings.camera.exposureValue));
    camera.setValue("frame_rate", QString::number(settings.camera.frameRate, 'g', 15));
    camera.setValue("camera_interface", settings.camera.cameraInterface);
    camera.setValue("camera_type", settings.camera.cameraType);
    camera.setValue("camera_id", QString::number(settings.camera.cameraId));
    camera.setValue("gpu_id", QString::number(settings.camera.gpuId));
    camera.setValue("missed_frame_threshold", QString::number(settings.camera.missedFrameThreshold));

    return pipeline.save(errorMessage)
        && pointCloud.save(errorMessage)
        && mesh.save(errorMessage)
        && camera.save(errorMessage);
}

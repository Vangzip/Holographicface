#include "PrintConfig.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace {

void clearError(QString* errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }
}

PrintAxisConfig readAxis(QSettings& settings, const QString& group, const PrintAxisConfig& defaults)
{
    settings.beginGroup(group);
    PrintAxisConfig axis;
    axis.subdivision = settings.value("subdivision", defaults.subdivision).toInt();
    axis.resolution = settings.value("resolution", defaults.resolution).toInt();
    axis.speedOfMovement = settings.value("speed_of_movement", static_cast<qlonglong>(defaults.speedOfMovement)).toLongLong();
    axis.acceleratedVelocity = settings.value("accelerated_velocity", static_cast<qlonglong>(defaults.acceleratedVelocity)).toLongLong();
    axis.startSpeed = settings.value("start_speed", static_cast<qlonglong>(defaults.startSpeed)).toLongLong();
    axis.stopSpeed = settings.value("stop_speed", static_cast<qlonglong>(defaults.stopSpeed)).toLongLong();
    axis.maxDistance = settings.value("max_distance", defaults.maxDistance).toDouble();
    axis.changeDirection = settings.value("change_direction", defaults.changeDirection).toBool();
    axis.electricalStatus = settings.value("electrical_status", defaults.electricalStatus).toBool();
    settings.endGroup();
    return axis;
}

void writeAxis(QSettings& settings, const QString& group, const PrintAxisConfig& axis)
{
    settings.beginGroup(group);
    settings.setValue("subdivision", axis.subdivision);
    settings.setValue("resolution", axis.resolution);
    settings.setValue("speed_of_movement", static_cast<qlonglong>(axis.speedOfMovement));
    settings.setValue("accelerated_velocity", static_cast<qlonglong>(axis.acceleratedVelocity));
    settings.setValue("start_speed", static_cast<qlonglong>(axis.startSpeed));
    settings.setValue("stop_speed", static_cast<qlonglong>(axis.stopSpeed));
    settings.setValue("max_distance", axis.maxDistance);
    settings.setValue("change_direction", axis.changeDirection);
    settings.setValue("electrical_status", axis.electricalStatus);
    settings.endGroup();
}

} // namespace

Print9030Config defaultPrint9030Config()
{
    Print9030Config config;

    config.axisX.subdivision = 40;
    config.axisX.resolution = 50;
    config.axisX.speedOfMovement = 5000;
    config.axisX.acceleratedVelocity = 50000;
    config.axisX.startSpeed = 500;
    config.axisX.stopSpeed = 500;
    config.axisX.maxDistance = 150.0;

    config.axisY.subdivision = 40;
    config.axisY.resolution = 50;
    config.axisY.speedOfMovement = 60000;
    config.axisY.acceleratedVelocity = 150000;
    config.axisY.startSpeed = 0;
    config.axisY.stopSpeed = 0;
    config.axisY.maxDistance = 150.0;

    config.axisZ.subdivision = 40;
    config.axisZ.resolution = 40;
    config.axisZ.speedOfMovement = 20000;
    config.axisZ.acceleratedVelocity = 200000;
    config.axisZ.startSpeed = 1000;
    config.axisZ.stopSpeed = 1000;
    config.axisZ.maxDistance = 150.0;

    config.axisW.subdivision = 20;
    config.axisW.resolution = 100;
    config.axisW.speedOfMovement = 10000;
    config.axisW.acceleratedVelocity = 10000;
    config.axisW.startSpeed = 1000;
    config.axisW.stopSpeed = 10000;
    config.axisW.maxDistance = 0.0;
    config.axisW.electricalStatus = false;

    config.main.moveAdjustMm = 20.0;
    config.main.widthScale = 3.8;
    config.main.heightScale = 2.8;
    config.main.addTempPulse = 16000;
    config.main.leadPulse = 1000;

    return config;
}

Print9030Config loadPrint9030Config(const QString& path, QString* errorMessage)
{
    clearError(errorMessage);
    Print9030Config config = defaultPrint9030Config();

    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return config;
    }

    QSettings settings(path, QSettings::IniFormat);
    settings.setIniCodec("UTF-8");

    settings.beginGroup("meta");
    const int savedVersion = settings.value("version", 1).toInt();
    settings.endGroup();

    settings.beginGroup("main");
    config.main.moveAdjustMm = settings.value("move_adjust_mm", config.main.moveAdjustMm).toDouble();
    config.main.rowSpacingMm = settings.value("row_spacing_mm", config.main.rowSpacingMm).toDouble();
    config.main.columnSpacingMm = settings.value("column_spacing_mm", config.main.columnSpacingMm).toDouble();
    config.main.gridRows = settings.value("grid_rows", config.main.gridRows).toInt();
    config.main.gridColumns = settings.value("grid_columns", config.main.gridColumns).toInt();
    config.main.delaySeconds = settings.value("delay_seconds", config.main.delaySeconds).toDouble();
    config.main.exposureSeconds = settings.value("exposure_seconds", config.main.exposureSeconds).toDouble();
    config.main.widthScale = settings.value("width_scale", config.main.widthScale).toDouble();
    config.main.heightScale = settings.value("height_scale", config.main.heightScale).toDouble();
    config.main.addTempPulse = settings.value("add_temp_pulse", static_cast<qlonglong>(config.main.addTempPulse)).toLongLong();
    config.main.leadPulse = settings.value("lead_pulse", static_cast<qlonglong>(config.main.leadPulse)).toLongLong();
    settings.endGroup();

    const Print9030Config defaults = defaultPrint9030Config();
    config.axisX = readAxis(settings, "axis_x", defaults.axisX);
    config.axisY = readAxis(settings, "axis_y", defaults.axisY);
    config.axisZ = readAxis(settings, "axis_z", defaults.axisZ);
    config.axisW = readAxis(settings, "axis_w", defaults.axisW);
    config.axisW.electricalStatus = false;
    Q_UNUSED(savedVersion);
    config.configVersion = 2;
    return config;
}

bool savePrint9030Config(const QString& path, const Print9030Config& config, QString* errorMessage)
{
    clearError(errorMessage);
    if (path.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "9030 print config path is empty.";
        }
        return false;
    }

    const QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath())) {
        if (errorMessage) {
            *errorMessage = "Cannot create config directory: " + info.absolutePath();
        }
        return false;
    }

    QSettings settings(path, QSettings::IniFormat);
    settings.setIniCodec("UTF-8");
    settings.clear();

    settings.beginGroup("meta");
    settings.setValue("version", 2);
    settings.endGroup();

    settings.beginGroup("main");
    settings.setValue("move_adjust_mm", config.main.moveAdjustMm);
    settings.setValue("row_spacing_mm", config.main.rowSpacingMm);
    settings.setValue("column_spacing_mm", config.main.columnSpacingMm);
    settings.setValue("grid_rows", config.main.gridRows);
    settings.setValue("grid_columns", config.main.gridColumns);
    settings.setValue("delay_seconds", config.main.delaySeconds);
    settings.setValue("exposure_seconds", config.main.exposureSeconds);
    settings.setValue("width_scale", config.main.widthScale);
    settings.setValue("height_scale", config.main.heightScale);
    settings.setValue("add_temp_pulse", static_cast<qlonglong>(config.main.addTempPulse));
    settings.setValue("lead_pulse", static_cast<qlonglong>(config.main.leadPulse));
    settings.endGroup();

    writeAxis(settings, "axis_x", config.axisX);
    writeAxis(settings, "axis_y", config.axisY);
    writeAxis(settings, "axis_z", config.axisZ);
    PrintAxisConfig axisW = config.axisW;
    axisW.electricalStatus = false;
    writeAxis(settings, "axis_w", axisW);
    settings.sync();

    if (settings.status() != QSettings::NoError) {
        if (errorMessage) {
            *errorMessage = "Cannot write 9030 print config: " + path;
        }
        return false;
    }
    return true;
}

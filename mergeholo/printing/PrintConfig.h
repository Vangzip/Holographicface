#pragma once

#include <QString>

struct PrintAxisConfig {
    int subdivision = 40;
    int resolution = 200;
    long speedOfMovement = 20000;
    long acceleratedVelocity = 200000;
    long startSpeed = 2000;
    long stopSpeed = 2000;
    double maxDistance = 150.0;
    bool changeDirection = false;
    bool electricalStatus = false;
};

struct PrintMainConfig {
    double moveAdjustMm = 5.0;
    double rowSpacingMm = 0.5;
    double columnSpacingMm = 0.5;
    int gridRows = 150;
    int gridColumns = 150;
    double delaySeconds = 0.0;
    double exposureSeconds = 0.5;
    double widthScale = 1.0;
    double heightScale = 1.0;
    long addTempPulse = 12800;
    long long leadPulse = 800;
};

struct Print9030Config {
    int configVersion = 2;
    PrintMainConfig main;
    PrintAxisConfig axisX;
    PrintAxisConfig axisY;
    PrintAxisConfig axisZ;
    PrintAxisConfig axisW;
};

Print9030Config defaultPrint9030Config();
Print9030Config loadPrint9030Config(const QString& path, QString* errorMessage = nullptr);
bool savePrint9030Config(const QString& path, const Print9030Config& config, QString* errorMessage = nullptr);

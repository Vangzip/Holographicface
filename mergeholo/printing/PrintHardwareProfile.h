#pragma once

#include <QString>
#include <QVector>

struct PrintHardwareProfile {
    enum class LogicalAxis { X, Y, Z, R };

    int version = 1;
    int cardIndex = 0;
    int axisX = 1;
    int axisY = 0;
    QVector<LogicalAxis> homeOrder = {LogicalAxis::Y, LogicalAxis::X};
    int homeDirectionX = -1;
    int homeDirectionY = -1;
    int homeSpeed = 32000;
    int homeAcceleration = 80000;
    int homeDeceleration = 80000;
    int homeTimeoutMs = 180000;
    int homeStableMs = 500;
    int homeMinimumMove = 100;
    int homeBackoffX = 28000;
    int homeBackoffY = 92000;
    int homeBackoffSpeed = 10000;
    int homeBackoffTimeoutMs = 30000;
    long printStepPulse = 1000;
    long forwardDelayPulse = 4000;
    long reverseFixedPulse = 2000;
    long exposureOffsetPulse = 2000;
    int sv660nDoFunction = 25;
    int sv660nPointIndex = 1;
    int sv660nMode = 0;
    int sv660nWidth = 1000;
    bool sv660nUserUnits = true;
    int sv660nPositiveAttribute = 129;
    int sv660nNegativeAttribute = 130;
};

PrintHardwareProfile loadPrintHardwareProfile(const QString& path, QString* errorMessage = nullptr);
bool validatePrintHardwareProfile(const PrintHardwareProfile& profile, QString* errorMessage = nullptr);

#pragma once

#include <QRect>
#include <QString>
#include <QVector>

#include <optional>

struct DisplayMonitor {
    QRect geometry;
    bool primary = false;
    bool attachedToDesktop = false;
    quintptr nativeMonitor = 0;
    QString deviceName;
    double refreshHz = 0.0;
};

QVector<DisplayMonitor> enumerateAttachedDesktopMonitors(QString* errorMessage = nullptr);
std::optional<int> selectV2SecondScreenIndex(const QVector<DisplayMonitor>& displays);

#include "SecondScreenSelection.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

std::optional<int> selectV2SecondScreenIndex(const QVector<DisplayMonitor>& displays)
{
    std::optional<int> selected;
    for (int index = 0; index < displays.size(); ++index) {
        const DisplayMonitor& display = displays.at(index);
        if (!display.primary && display.attachedToDesktop && display.geometry.isValid()
            && display.nativeMonitor != 0) {
            selected = index;
        }
    }
    return selected;
}

QVector<DisplayMonitor> enumerateAttachedDesktopMonitors(QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
    QVector<DisplayMonitor> displays;
#ifdef Q_OS_WIN
    const BOOL ok = EnumDisplayMonitors(nullptr, nullptr,
        [](HMONITOR monitor, HDC, LPRECT, LPARAM context) -> BOOL {
            auto* result = reinterpret_cast<QVector<DisplayMonitor>*>(context);
            MONITORINFOEXW info = {};
            info.cbSize = sizeof(info);
            if (!GetMonitorInfoW(monitor, &info)) return TRUE;
            const RECT& r = info.rcMonitor;
            DisplayMonitor display;
            display.geometry = QRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
            display.primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
            display.attachedToDesktop = true;
            display.nativeMonitor = reinterpret_cast<quintptr>(monitor);
            display.deviceName = QString::fromWCharArray(info.szDevice);
            DEVMODEW mode = {};
            mode.dmSize = sizeof(mode);
            if (EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode)
                && mode.dmDisplayFrequency > 1) {
                display.refreshHz = static_cast<double>(mode.dmDisplayFrequency);
            }
            result->push_back(display);
            return TRUE;
        }, reinterpret_cast<LPARAM>(&displays));
    if (!ok && errorMessage) {
        *errorMessage = QStringLiteral("EnumDisplayMonitors failed (Win32=%1).").arg(GetLastError());
    }
#else
    if (errorMessage) *errorMessage = QStringLiteral("V2 physical display presentation is Windows-only.");
#endif
    return displays;
}

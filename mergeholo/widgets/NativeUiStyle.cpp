#include "NativeUiStyle.h"

#include <QApplication>
#include <QFont>
#include <QStyle>
#include <QStyleFactory>

void applyNativeWindowsUiStyle(QApplication& application)
{
#ifdef Q_OS_WIN
    if (QStyle* style = QStyleFactory::create("windowsvista")) {
        application.setStyle(style);
    }
#endif

    QFont font("Microsoft YaHei UI");
    font.setPointSize(9);
    application.setFont(font);
}

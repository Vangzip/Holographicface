#pragma once

#include <QByteArray>

enum class PrintPixelFormat {
    Bgr24,
    Bgra32
};

struct PrintFrame {
    QByteArray pixels;
    int width = 0;
    int height = 0;
    int stride = 0;
    PrintPixelFormat format = PrintPixelFormat::Bgr24;

    bool isValid() const;
};

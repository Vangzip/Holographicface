#include "PrintFrame.h"

#include <limits>

bool PrintFrame::isValid() const
{
    if (pixels.isNull() || pixels.isEmpty() || width <= 0 || height <= 0 || stride <= 0) {
        return false;
    }

    int bytesPerPixel = 0;
    switch (format) {
    case PrintPixelFormat::Bgr24: bytesPerPixel = 3; break;
    case PrintPixelFormat::Bgra32: bytesPerPixel = 4; break;
    default: return false;
    }

    const quint64 rowBytes = static_cast<quint64>(width) * static_cast<quint64>(bytesPerPixel);
    if (rowBytes > static_cast<quint64>(std::numeric_limits<int>::max())
        || static_cast<quint64>(stride) < rowBytes) {
        return false;
    }
    const quint64 requiredBytes = static_cast<quint64>(height - 1)
        * static_cast<quint64>(stride) + rowBytes;
    return requiredBytes > 0 && requiredBytes <= static_cast<quint64>(pixels.size());
}

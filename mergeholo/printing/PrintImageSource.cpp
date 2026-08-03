#include "PrintImageSource.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <limits>

namespace {

void clearError(QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
}

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage) *errorMessage = message;
}

bool decodeFrame(const QString& path, PrintFrame* destination, QString* errorMessage)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage decoded = reader.read();
    if (decoded.isNull()) {
        setError(errorMessage, "Cannot decode print image: " + path + " (" + reader.errorString() + ")");
        return false;
    }
    const QImage bgr = decoded.convertToFormat(QImage::Format_BGR888);
    const qint64 rowBytes64 = static_cast<qint64>(bgr.width()) * 3;
    const qint64 packedBytes64 = rowBytes64 * bgr.height();
    if (rowBytes64 <= 0 || packedBytes64 <= 0
        || packedBytes64 > std::numeric_limits<int>::max()) {
        setError(errorMessage, "Decoded print image has unsupported dimensions: " + path);
        return false;
    }
    PrintFrame frame;
    frame.width = bgr.width();
    frame.height = bgr.height();
    frame.stride = static_cast<int>(rowBytes64);
    frame.format = PrintPixelFormat::Bgr24;
    frame.pixels.resize(static_cast<int>(packedBytes64));
    for (int row = 0; row < frame.height; ++row) {
        std::memcpy(frame.pixels.data() + row * frame.stride,
            bgr.constScanLine(row), static_cast<size_t>(frame.stride));
    }
    *destination = std::move(frame);
    return true;
}

struct FolderImageEntry {
    QFileInfo info;
    int row = 0;
    int column = 0;
};

bool inferGridOrder(QVector<FolderImageEntry>* entries, int* rows, int* columns)
{
    if (!entries || entries->isEmpty() || !rows || !columns) return false;
    const QRegularExpression namePattern(QStringLiteral("^(\\d{3})(\\d{3})$"));
    QSet<quint64> cells;
    int maxRow = 0;
    int maxColumn = 0;
    for (FolderImageEntry& entry : *entries) {
        const QRegularExpressionMatch match =
            namePattern.match(entry.info.completeBaseName());
        if (!match.hasMatch()) return false;
        entry.row = match.captured(1).toInt();
        entry.column = match.captured(2).toInt();
        if (entry.row <= 0 || entry.column <= 0) return false;
        const quint64 cell = (static_cast<quint64>(entry.row) << 32)
            | static_cast<quint32>(entry.column);
        if (cells.contains(cell)) return false;
        cells.insert(cell);
        maxRow = qMax(maxRow, entry.row);
        maxColumn = qMax(maxColumn, entry.column);
    }
    const qint64 expectedCount = static_cast<qint64>(maxRow) * maxColumn;
    if (expectedCount != entries->size()) return false;
    std::sort(entries->begin(), entries->end(),
        [](const FolderImageEntry& left, const FolderImageEntry& right) {
            return left.row == right.row ? left.column < right.column
                                         : left.row < right.row;
        });
    *rows = maxRow;
    *columns = maxColumn;
    return true;
}

} // namespace

PrintImageSet PrintImageSet::fromFrames(const QVector<PrintFrame>& frames,
    QString* errorMessage, PrintImageSourceType sourceType, const QString& sourcePath)
{
    clearError(errorMessage);
    if (frames.isEmpty()) {
        setError(errorMessage, "Immutable print frame set is empty.");
        return {};
    }
    for (int index = 0; index < frames.size(); ++index) {
        if (!frames[index].isValid()) {
            setError(errorMessage, QString("Immutable print frame %1 is invalid or truncated.").arg(index));
            return {};
        }
    }
    PrintImageSet result;
    result.frames_ = frames;
    result.sourceType_ = sourceType;
    result.sourcePath_ = sourcePath;
    return result;
}

size_t PrintImageSet::imageCount() const
{
    return static_cast<size_t>(frames_.size());
}

bool PrintImageSet::isValid() const
{
    if (frames_.isEmpty()) return false;
    for (const PrintFrame& frame : frames_) if (!frame.isValid()) return false;
    return true;
}

bool PrintImageSet::copyImageBytes(size_t index, QByteArray* destination) const
{
    if (!destination || index >= imageCount()) return false;
    *destination = frames_.at(static_cast<int>(index)).pixels;
    return true;
}

bool PrintImageSet::copyFrame(size_t index, PrintFrame* destination,
    QString* errorMessage) const
{
    clearError(errorMessage);
    if (!destination) {
        setError(errorMessage, "Print frame destination is unavailable.");
        return false;
    }
    *destination = {};
    if (index >= imageCount()) {
        setError(errorMessage, "Print frame index is outside the immutable set.");
        return false;
    }
    const PrintFrame& frame = frames_.at(static_cast<int>(index));
    if (!frame.isValid()) {
        setError(errorMessage, "Stored immutable print frame is invalid or truncated.");
        return false;
    }
    *destination = frame;
    return true;
}

PrintImageSourceType PrintImageSet::sourceType() const { return sourceType_; }
QString PrintImageSet::sourcePath() const { return sourcePath_; }

PrintImageSet makePrintImageSetFromElementalMemory(
    const ElementalMemoryResult& result, QString* errorMessage)
{
    clearError(errorMessage);
    const size_t expectedBytes = result.rows > 0 && result.cols > 0
        ? static_cast<size_t>(result.rows) * static_cast<size_t>(result.cols) * 3U : 0U;
    if (!result.hasResult() || result.imageCount == 0 || result.imageBytes != expectedBytes
        || expectedBytes > static_cast<size_t>(std::numeric_limits<int>::max())) {
        setError(errorMessage, "Elemental memory result is empty or malformed.");
        return {};
    }
    QVector<PrintFrame> frames;
    if (result.imageCount > static_cast<size_t>(std::numeric_limits<int>::max())) {
        setError(errorMessage, "Elemental image count exceeds the supported immutable set.");
        return {};
    }
    frames.reserve(static_cast<int>(result.imageCount));
    for (size_t index = 0; index < result.imageCount; ++index) {
        PrintFrame frame;
        frame.width = result.cols;
        frame.height = result.rows;
        frame.stride = result.cols * 3;
        frame.format = PrintPixelFormat::Bgr24;
        frame.pixels.resize(static_cast<int>(result.imageBytes));
        if (!result.copyImage(index,
                reinterpret_cast<unsigned char*>(frame.pixels.data()))) {
            setError(errorMessage, QString("Cannot copy elemental image %1.").arg(static_cast<qulonglong>(index)));
            return {};
        }
        for (int pixel = 0; pixel < frame.pixels.size(); pixel += 3) {
            const char red = frame.pixels[pixel];
            frame.pixels[pixel] = frame.pixels[pixel + 2];
            frame.pixels[pixel + 2] = red;
        }
        frames.append(std::move(frame));
    }
    return PrintImageSet::fromFrames(frames, errorMessage,
        PrintImageSourceType::ElementalMemory);
}

PrintImageSet makePrintImageSetFromElementalMemory(
    std::shared_ptr<const ElementalMemoryResult> result, QString* errorMessage)
{
    if (!result) {
        setError(errorMessage, "Elemental memory result is unavailable.");
        return {};
    }
    return makePrintImageSetFromElementalMemory(*result, errorMessage);
}

PrintImageFolderLoadResult loadPrintImagesFromFolderWithGridInfo(const QString& folderPath,
    QString* errorMessage)
{
    clearError(errorMessage);
    PrintImageFolderLoadResult result;
    const QFileInfo info(folderPath);
    if (folderPath.isEmpty() || !info.isDir()) {
        setError(errorMessage, "Image folder does not exist: " + folderPath);
        return result;
    }
    QDir dir(info.absoluteFilePath());
    const QStringList filters = {"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.tif", "*.tiff"};
    const QFileInfoList fileEntries = dir.entryInfoList(filters,
        QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);
    QVector<FolderImageEntry> entries;
    entries.reserve(fileEntries.size());
    for (const QFileInfo& entry : fileEntries) entries.append({entry});
    if (entries.isEmpty()) {
        setError(errorMessage, "Image folder has no supported images: " + folderPath);
        return result;
    }
    if (!inferGridOrder(&entries, &result.gridRows, &result.gridColumns)) {
        result.gridWarning = QStringLiteral(
            "Image names do not form a complete RRRCCC grid; keep manual row and column values.");
    }
    QVector<PrintFrame> frames;
    frames.reserve(entries.size());
    for (const FolderImageEntry& entry : entries) {
        PrintFrame frame;
        if (!decodeFrame(entry.info.absoluteFilePath(), &frame, errorMessage)) return {};
        frames.append(std::move(frame));
    }
    result.images = PrintImageSet::fromFrames(frames, errorMessage,
        PrintImageSourceType::Folder, dir.absolutePath());
    return result;
}

PrintImageSet loadPrintImagesFromFolder(const QString& folderPath,
    QString* errorMessage)
{
    return loadPrintImagesFromFolderWithGridInfo(folderPath, errorMessage).images;
}

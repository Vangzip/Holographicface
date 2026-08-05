#include "PrintImageSource.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <atlimage.h>
#endif

#include <algorithm>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <limits>
#include <mutex>
#include <thread>

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
#ifdef Q_OS_WIN
    CImage decoded;
    const HRESULT hr = decoded.Load(
        reinterpret_cast<LPCWSTR>(path.utf16()));
    if (FAILED(hr)) {
        setError(errorMessage, "Cannot decode print image: " + path);
        return false;
    }
    const int bytesPerPixel = decoded.GetBPP() / 8;
    const int width = decoded.GetWidth();
    const int height = decoded.GetHeight();
    const int pitch = decoded.GetPitch();
    const auto* bits = reinterpret_cast<const unsigned char*>(decoded.GetBits());
    if ((decoded.GetBPP() != 24 && decoded.GetBPP() != 32)
        || width <= 0 || height <= 0 || pitch == 0 || !bits) {
        setError(errorMessage, "Unsupported V2 CImage frame: " + path);
        return false;
    }
    const qint64 rowBytes64 = static_cast<qint64>(width) * bytesPerPixel;
    const qint64 packedBytes64 = rowBytes64 * height;
    if (packedBytes64 <= 0 || packedBytes64 > std::numeric_limits<int>::max()) {
        setError(errorMessage, "Decoded print image has unsupported dimensions: " + path);
        return false;
    }
    PrintFrame frame;
    frame.width = width;
    frame.height = height;
    frame.stride = static_cast<int>(rowBytes64);
    frame.format = decoded.GetBPP() == 24
        ? PrintPixelFormat::Bgr24 : PrintPixelFormat::Bgra32;
    frame.pixels.resize(static_cast<int>(packedBytes64));
    for (int row = 0; row < height; ++row) {
        std::memcpy(frame.pixels.data() + row * frame.stride,
            bits + static_cast<ptrdiff_t>(row) * pitch,
            static_cast<size_t>(frame.stride));
    }
    *destination = std::move(frame);
    return true;
#else
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
#endif
}

bool discoverV2FolderFiles(const QDir& directory, QStringList* files,
    QString* errorMessage)
{
    if (!files) {
        setError(errorMessage, "Print image folder discovery destination is unavailable.");
        return false;
    }
    files->clear();
#ifdef Q_OS_WIN
    const QByteArray pattern = QDir::toNativeSeparators(
        directory.absoluteFilePath(QStringLiteral("*.*"))).toLocal8Bit();
    WIN32_FIND_DATAA data {};
    HANDLE search = FindFirstFileA(pattern.constData(), &data);
    if (search == INVALID_HANDLE_VALUE) {
        setError(errorMessage, "Cannot enumerate image folder: "
            + directory.absolutePath());
        return false;
    }
    do {
        const QString name = QString::fromLocal8Bit(data.cFileName);
        if (name == QStringLiteral(".") || name == QStringLiteral("..")
            || (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            continue;
        }
        files->append(directory.absoluteFilePath(name));
    } while (FindNextFileA(search, &data));
    FindClose(search);
    return true;
#else
    Q_UNUSED(directory);
    setError(errorMessage,
        "V2 folder discovery requires Windows FindFirstFile/FindNextFile semantics.");
    return false;
#endif
}

class BoundedPrintImageQueue final : public IPrintImageQueue
{
public:
    BoundedPrintImageQueue(QVector<PrintFrame> frames, QStringList files)
        : frames_(std::move(frames))
        , files_(std::move(files))
    {
    }

    ~BoundedPrintImageQueue() override
    {
        stop();
    }

    bool start(QString* errorMessage) override
    {
        clearError(errorMessage);
        std::unique_lock<std::mutex> lock(mutex_);
        if (started_) return true;
        if (producer_.joinable()) {
            setError(errorMessage, "Print image queue producer was not joined.");
            return false;
        }
        if (!producerError_.isEmpty()) {
            setError(errorMessage, producerError_);
            return false;
        }
        stopRequested_ = false;
        started_ = true;
        const size_t consumedCount = nextIndex_ >= queued_.size()
            ? nextIndex_ - queued_.size() : 0;
        const size_t remainingCount = sourceCount()
            - std::min(consumedCount, sourceCount());
        const size_t preloadTarget = std::min(
            static_cast<size_t>(900), remainingCount);
        if (producerFinished_ || nextIndex_ >= sourceCount()) {
            producerFinished_ = true;
            condition_.notify_all();
        } else {
            producer_ = std::thread([this] { produce(); });
        }
        condition_.wait(lock, [this, preloadTarget] {
            return queued_.size() >= preloadTarget
                || !producerError_.isEmpty() || producerFinished_;
        });
        if (!producerError_.isEmpty()) {
            setError(errorMessage, producerError_);
            return false;
        }
        if (queued_.size() < preloadTarget) {
            setError(errorMessage,
                "Print image source ended before the V2 preload target was available.");
            return false;
        }
        return true;
    }

    bool takeRow(int columnCount, QVector<PrintFrame>* row,
        QString* errorMessage) override
    {
        clearError(errorMessage);
        if (!row || columnCount <= 0) {
            setError(errorMessage, "Print image queue row request is invalid.");
            return false;
        }
        row->clear();
        std::unique_lock<std::mutex> lock(mutex_);
        if (!started_) {
            setError(errorMessage, "Print image queue is not started.");
            return false;
        }
        condition_.wait(lock, [this, columnCount] {
            return static_cast<int>(queued_.size()) >= columnCount
                || !producerError_.isEmpty() || producerFinished_
                || stopRequested_;
        });
        if (static_cast<int>(queued_.size()) < columnCount) {
            if (!producerError_.isEmpty()) {
                setError(errorMessage, producerError_);
            } else if (stopRequested_) {
                setError(errorMessage, "Print image queue was stopped.");
            } else {
                setError(errorMessage,
                    QString("Print image source ended before a complete row of %1 frames.")
                        .arg(columnCount));
            }
            return false;
        }
        row->reserve(columnCount);
        for (int column = 0; column < columnCount; ++column) {
            row->append(std::move(queued_.front()));
            queued_.pop_front();
        }
        condition_.notify_all();
        return true;
    }

    void stop() override
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopRequested_ = true;
            started_ = false;
            condition_.notify_all();
        }
        if (producer_.joinable()) producer_.join();
    }

private:
    size_t sourceCount() const
    {
        return files_.isEmpty()
            ? static_cast<size_t>(frames_.size())
            : static_cast<size_t>(files_.size());
    }

    bool readFrame(size_t index, PrintFrame* frame, QString* errorMessage)
    {
        if (!files_.isEmpty()) {
            return decodeFrame(files_.at(static_cast<int>(index)), frame,
                errorMessage);
        }
        if (index >= static_cast<size_t>(frames_.size())) {
            setError(errorMessage, "Print image queue index is outside the source.");
            return false;
        }
        *frame = frames_.at(static_cast<int>(index));
        return frame->isValid();
    }

    void produce()
    {
#ifdef Q_OS_WIN
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
        for (;;) {
            size_t index = 0;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] {
                    return stopRequested_
                        || static_cast<int>(queued_.size()) < 900;
                });
                if (stopRequested_) return;
                if (nextIndex_ >= sourceCount()) {
                    producerFinished_ = true;
                    condition_.notify_all();
                    return;
                }
                index = nextIndex_;
            }

            PrintFrame frame;
            QString error;
            if (!readFrame(index, &frame, &error)) {
                std::lock_guard<std::mutex> lock(mutex_);
                producerError_ = error.isEmpty()
                    ? QString("Cannot read print image %1.").arg(
                        static_cast<qulonglong>(index))
                    : error;
                producerFinished_ = true;
                condition_.notify_all();
                return;
            }

            {
                std::lock_guard<std::mutex> lock(mutex_);
                queued_.push_back(std::move(frame));
                ++nextIndex_;
                condition_.notify_all();
                if (stopRequested_) return;
            }
        }
    }

    const QVector<PrintFrame> frames_;
    const QStringList files_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<PrintFrame> queued_;
    std::thread producer_;
    size_t nextIndex_ = 0;
    bool started_ = false;
    bool stopRequested_ = false;
    bool producerFinished_ = false;
    QString producerError_;
};

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

PrintImageSet PrintImageSet::fromFolderFiles(const QStringList& files,
    const QString& sourcePath, QString* errorMessage)
{
    clearError(errorMessage);
    if (files.isEmpty()) {
        setError(errorMessage, "Print image folder file list is empty.");
        return {};
    }
    PrintImageSet result;
    result.files_ = files;
    result.sourceType_ = PrintImageSourceType::Folder;
    result.sourcePath_ = sourcePath;
    return result;
}

size_t PrintImageSet::imageCount() const
{
    return files_.isEmpty()
        ? static_cast<size_t>(frames_.size())
        : static_cast<size_t>(files_.size());
}

bool PrintImageSet::isValid() const
{
    if (!files_.isEmpty()) return sourceType_ == PrintImageSourceType::Folder;
    if (frames_.isEmpty()) return false;
    for (const PrintFrame& frame : frames_) if (!frame.isValid()) return false;
    return true;
}

bool PrintImageSet::copyImageBytes(size_t index, QByteArray* destination) const
{
    if (!destination || index >= imageCount()) return false;
    if (!files_.isEmpty()) {
        PrintFrame frame;
        if (!copyFrame(index, &frame)) return false;
        *destination = frame.pixels;
        return true;
    }
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
    if (!files_.isEmpty()) {
        return decodeFrame(files_.at(static_cast<int>(index)), destination,
            errorMessage);
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

std::unique_ptr<IPrintImageQueue> PrintImageSet::createQueue() const
{
    if (!isValid()) return {};
    return std::make_unique<BoundedPrintImageQueue>(frames_, files_);
}

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
    QStringList files;
    if (!discoverV2FolderFiles(dir, &files, errorMessage)) return result;
    if (files.isEmpty()) {
        setError(errorMessage, "Image folder has no files: " + folderPath);
        return result;
    }
    result.images = PrintImageSet::fromFolderFiles(
        files, dir.absolutePath(), errorMessage);
    const QString filename = QFileInfo(files.last()).fileName();
    const int jpgPos = filename.indexOf(QStringLiteral(".jpg"));
    const QString stem = jpgPos < 0 ? filename : filename.left(jpgPos);
    const int half = stem.size() / 2;
    const auto v2Atoi = [](const QString& value) {
        const QByteArray local = value.toLocal8Bit();
        return static_cast<int>(std::strtol(local.constData(), nullptr, 10));
    };
    result.gridRows = v2Atoi(stem.left(half));
    result.gridColumns = v2Atoi(stem.mid(half, half));
    return result;
}

PrintImageSet loadPrintImagesFromFolder(const QString& folderPath,
    QString* errorMessage)
{
    return loadPrintImagesFromFolderWithGridInfo(folderPath, errorMessage).images;
}

#pragma once

#include "elemental/ElementalMemoryResult.h"
#include "PrintFrame.h"

#include <QString>
#include <QStringList>
#include <QVector>

#include <cstddef>
#include <functional>
#include <memory>

class IPrintImageQueue
{
public:
    virtual ~IPrintImageQueue() = default;
    virtual bool start(QString* errorMessage = nullptr) = 0;
    virtual bool takeRow(int columnCount, QVector<PrintFrame>* row,
        QString* errorMessage = nullptr) = 0;
    // stop() also joins the background producer before it returns.
    virtual void stop() = 0;
};

enum class PrintImageSourceType {
    None,
    ElementalMemory,
    Folder,
    ImmutableFrames
};

class PrintImageSet {
public:
    static PrintImageSet fromFrames(const QVector<PrintFrame>& frames,
        QString* errorMessage = nullptr,
        PrintImageSourceType sourceType = PrintImageSourceType::ImmutableFrames,
        const QString& sourcePath = QString());
    static PrintImageSet fromFolderFiles(const QStringList& files,
        const QString& sourcePath = QString(),
        QString* errorMessage = nullptr);

    size_t imageCount() const;
    bool isValid() const;
    bool copyImageBytes(size_t index, QByteArray* destination) const;
    bool copyFrame(size_t index, PrintFrame* destination,
        QString* errorMessage = nullptr) const;
    PrintImageSourceType sourceType() const;
    QString sourcePath() const;
    std::unique_ptr<IPrintImageQueue> createQueue() const;

private:
    QVector<PrintFrame> frames_;
    QStringList files_;
    PrintImageSourceType sourceType_ = PrintImageSourceType::None;
    QString sourcePath_;
};

using PrintImageQueueFactory =
    std::function<std::unique_ptr<IPrintImageQueue>(const PrintImageSet&)>;

struct PrintImageFolderLoadResult {
    PrintImageSet images;
    int gridRows = 0;
    int gridColumns = 0;
    QString gridWarning;

    bool hasInferredGrid() const { return gridRows > 0 && gridColumns > 0; }
};

PrintImageSet makePrintImageSetFromElementalMemory(const ElementalMemoryResult& result,
    QString* errorMessage = nullptr);
PrintImageSet makePrintImageSetFromElementalMemory(
    std::shared_ptr<const ElementalMemoryResult> result,
    QString* errorMessage = nullptr);
PrintImageFolderLoadResult loadPrintImagesFromFolderWithGridInfo(
    const QString& folderPath, QString* errorMessage = nullptr);
PrintImageSet loadPrintImagesFromFolder(const QString& folderPath,
    QString* errorMessage = nullptr);

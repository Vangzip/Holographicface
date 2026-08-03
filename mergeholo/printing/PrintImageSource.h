#pragma once

#include "elemental/ElementalMemoryResult.h"
#include "PrintFrame.h"

#include <QString>
#include <QStringList>
#include <QVector>

#include <cstddef>
#include <memory>

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

    size_t imageCount() const;
    bool isValid() const;
    bool copyImageBytes(size_t index, QByteArray* destination) const;
    bool copyFrame(size_t index, PrintFrame* destination,
        QString* errorMessage = nullptr) const;
    PrintImageSourceType sourceType() const;
    QString sourcePath() const;

private:
    QVector<PrintFrame> frames_;
    PrintImageSourceType sourceType_ = PrintImageSourceType::None;
    QString sourcePath_;
};

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

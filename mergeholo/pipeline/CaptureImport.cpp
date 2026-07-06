#include "CaptureImport.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QTextStream>

namespace {

QString withSlash(QString path)
{
    if (!path.endsWith('/') && !path.endsWith('\\')) {
        path += '/';
    }
    return path;
}

} // namespace

int importCaptureForPipeline(const CaptureImportOptions& options)
{
    const QString captureRoot = withSlash(QDir(options.captureRoot).absolutePath());
    const QString source2d = captureRoot + "2d/";
    const QString source3d = captureRoot + "3d/";
    const QString targetRoot = withSlash(QDir(options.pipelineInputDir).absolutePath());

    QTextStream out(stdout);
    QTextStream err(stderr);

    if (!QDir(source2d).exists()) {
        err << "[import] 2d directory not found: " << source2d << Qt::endl;
        return 1;
    }
    if (!QDir(source3d).exists()) {
        err << "[import] 3d directory not found: " << source3d << Qt::endl;
        return 1;
    }
    if (!QDir().mkpath(targetRoot)) {
        err << "[import] cannot create pipeline input directory: " << targetRoot << Qt::endl;
        return 1;
    }

    QStringList filters;
    filters << "*.jpg" << "*.JPG" << "*.jpeg" << "*.JPEG";
    const QFileInfoList rgbFiles = QDir(source2d).entryInfoList(filters, QDir::Files, QDir::Name);

    int imported = 0;
    int skipped = 0;
    int failed = 0;
    for (const QFileInfo& rgbInfo : rgbFiles) {
        const QString stem = rgbInfo.completeBaseName();
        const QString depthSource = source3d + stem + "_3D.tiff";
        if (!QFileInfo::exists(depthSource)) {
            ++skipped;
            err << "[import] missing 3d pair for " << rgbInfo.fileName() << Qt::endl;
            continue;
        }

        const QString rgbTarget = targetRoot + stem + ".jpg";
        const QString depthTarget = targetRoot + stem + ".tiff";
        if (options.overwrite) {
            QFile::remove(rgbTarget);
            QFile::remove(depthTarget);
        }

        const bool rgbOk = QFile::exists(rgbTarget) || QFile::copy(rgbInfo.absoluteFilePath(), rgbTarget);
        const bool depthOk = QFile::exists(depthTarget) || QFile::copy(depthSource, depthTarget);
        if (!rgbOk || !depthOk) {
            ++failed;
            err << "[import] copy failed for " << stem << Qt::endl;
            continue;
        }

        ++imported;
    }

    out << "[import] imported=" << imported
        << ", skipped=" << skipped
        << ", failed=" << failed
        << ", target=" << targetRoot << Qt::endl;
    return failed == 0 ? 0 : 1;
}

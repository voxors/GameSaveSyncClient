#include "utilFileSystem.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#include <miniz.h>
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include <miniz.h>
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4505)
#include <miniz.h>
#pragma warning(pop)
#else
#include <miniz.h>
#endif

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMutexLocker>
#include <QStandardPaths>

namespace utilFileSystem {
bool validatePath(const QString path) { return !getBasePath(path).isEmpty(); }

QString getUploadZipLocation() {
    QString tempDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        "/GameSaveSyncClient/upload/zip";
    QDir(tempDir).mkpath(".");
    return tempDir;
}

QString getDownloadZipLocation() {
    QString tempDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        "/GameSaveSyncClient/download/zip";
    QDir(tempDir).mkpath(".");
    return tempDir;
}

QString getUnzippedLocation(int pathId) {
    QString tempDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        "/GameSaveSyncClient/download/" + QString::number(pathId);
    QDir(tempDir).mkpath(".");
    return tempDir;
}

QString getBasePath(const QString path) {
    if (path.isEmpty()) {
        return {""};
    }

    QString normalizedPath = QDir::fromNativeSeparators(path);
    QString basePath = normalizedPath;

    if (basePath.contains('*') || basePath.contains('?')) {
        qsizetype starPos = basePath.indexOf('*');
        qsizetype questionPos = basePath.indexOf('?');
        qsizetype wildcardPos = -1;

        if (starPos != -1 && questionPos != -1) {
            wildcardPos = qMin(starPos, questionPos);
        } else if (starPos != -1) {
            wildcardPos = starPos;
        } else if (questionPos != -1) {
            wildcardPos = questionPos;
        }

        if (wildcardPos != -1) {
            qsizetype lastSeparator = basePath.lastIndexOf('/', wildcardPos);
            if (lastSeparator != -1) {
                basePath = basePath.left(lastSeparator);
            } else {
                basePath = ".";
            }
        }
    }

    while (basePath.endsWith('/') && basePath.length() > 1) {
        basePath.chop(1);
    }

    return basePath;
}

std::vector<FileHash> getHashFiles(const std::vector<QString>& filePaths,
                                   const QString& basePath) {
    std::vector<FileHash> hashes;

    for (const auto& filePath : filePaths) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Cannot open file for hashing:" << filePath;
            continue;
        }

        QByteArray data = file.readAll();
        file.close();

        QCryptographicHash fileHash(QCryptographicHash::Sha256);
        fileHash.addData(data);
        QByteArray hex = fileHash.result().toHex();

        QString relative = QDir(basePath).relativeFilePath(filePath);
        hashes.push_back(FileHash{.relativePath = relative, .hash = hex});
    }

    return hashes;
}

std::vector<QString> listFiles(const QString basePath, const QString pattern) {
    std::vector<QString> filePaths;
    QDirIterator it(basePath, QStringList() << pattern, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        filePaths.push_back(it.next());
    }
    return filePaths;
}

QString extractPattern(const QString fullPath) {
    QString basePath = getBasePath(fullPath);
    QString pattern = fullPath;
    if (!basePath.isEmpty())
        pattern.remove(0, basePath.length());
    if (pattern.startsWith('/'))
        pattern.remove(0, 1);
    if (pattern.isEmpty())
        pattern = "*";
    return pattern;
}

bool unzipZipAtDownload(const int pathId) {
    QString tempDir = getDownloadZipLocation();
    QString zipFile = QDir(tempDir).filePath(QString::number(pathId) + ".zip");
    QString zipDestination = getUnzippedLocation(pathId);
    QDir zipDestDir(zipDestination);
    zipDestDir.removeRecursively();
    zipDestDir.mkpath(".");

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, zipFile.toStdString().c_str(), 0))
        return false;

    for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip); i++) {
        mz_zip_archive_file_stat fileInfo;
        memset(&fileInfo, 0, sizeof(fileInfo));
        mz_zip_reader_file_stat(&zip, i, &fileInfo);
        if (fileInfo.m_is_directory) {
            QDir(zipDestDir.filePath(fileInfo.m_filename)).mkpath(".");
            continue;
        }
        QString relativePath(fileInfo.m_filename);
        if (relativePath.contains("/")) {
            relativePath = relativePath.left(relativePath.lastIndexOf("/"));
            QDir(zipDestDir.filePath(relativePath)).mkpath(".");
        }

        if (!mz_zip_reader_extract_to_file(
                &zip, i,
                zipDestDir.filePath(fileInfo.m_filename).toStdString().c_str(),
                0)) {
            return false;
        }
    }

    mz_zip_reader_end(&zip);

    return true;
}

std::vector<FileHash> createZipForUpload(const int pathId, const QString path) {
    QString basePath = getBasePath(path);
    QString pattern = extractPattern(path);

    QString tempDir = getUploadZipLocation();

    QFileInfo baseInfo(basePath);
    QString zipName = QString::number(pathId) + ".zip";
    QString zipPath = QDir(tempDir).filePath(zipName);

    std::vector<FileHash> hashes;

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_writer_init_file(&zip, zipPath.toUtf8().constData(), 0)) {
        qWarning() << "Failed to create zip file at" << zipPath;
        return hashes;
    }

    auto filePaths = listFiles(basePath, pattern);

    hashes = getHashFiles(filePaths, basePath);

    for (const auto& filePath : filePaths) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Cannot open file for reading:" << filePath;
            continue;
        }

        QByteArray data = file.readAll();
        file.close();

        QByteArray entryNameUtf8 =
            QDir(basePath).relativeFilePath(filePath).toUtf8();
        QByteArray filePathUtf8 = filePath.toUtf8();

        if (!mz_zip_writer_add_file(&zip, entryNameUtf8.constData(),
                                    filePathUtf8.constData(), nullptr, 0,
                                    MZ_BEST_COMPRESSION)) {
            qWarning() << "Failed to add file to zip:" << filePath << "as"
                       << entryNameUtf8.constData();
        }
    }

    if (!mz_zip_writer_finalize_archive(&zip)) {
        qWarning() << "Failed to finalize zip archive:" << zipPath;
    }
    mz_zip_writer_end(&zip);

    return hashes;
}

bool writeFileToFileSystemAtDownload(const int pathId, const QByteArray data) {
    QString tempDir = getDownloadZipLocation();

    QFile zipFile(QDir(tempDir).filePath(QString::number(pathId) + ".zip"));
    if (!zipFile.open(QFile::WriteOnly | QFile::Truncate))
        return false;

    zipFile.write(data);

    return true;
}

bool replaceFileAtDestination(const int pathId, const QString destination) {
    if (!QDir(getUnzippedLocation(pathId)).exists())
        return false;

    for (const QString& fileString :
         listFiles(getBasePath(destination), extractPattern(destination))) {
        QFile file(fileString);
        if (file.exists())
            file.remove();
    }

    QDirIterator files(QDir(getUnzippedLocation(pathId)).absolutePath(),
                       QDir::Files, QDirIterator::Subdirectories);
    while (files.hasNext()) {
        QString filePath = files.next();
        QFileInfo fileInfo(filePath);
        if (!fileInfo.isFile())
            continue;

        QString relativeFilePath =
            QDir(getUnzippedLocation(pathId)).relativeFilePath(filePath);
        QString relativePath =
            QDir(getUnzippedLocation(pathId))
                .relativeFilePath(QFileInfo(filePath).path());

        QDir(QDir(destination).absoluteFilePath(relativePath)).mkpath(".");
        if (!QFile::copy(
                fileInfo.absoluteFilePath(),
                QDir(destination).absoluteFilePath(relativeFilePath))) {
            return false;
        }
    }

    return true;
}

} // namespace utilFileSystem

#pragma once

#include <QString>

namespace utilFileSystem {
struct FileHash {
    QString relativePath;
    QString hash;

    bool operator==(const FileHash& other) const noexcept {
        return relativePath == other.relativePath && hash == other.hash;
    }

    bool operator!=(const FileHash& other) const noexcept {
        return !(*this == other);
    }
};

QString getUploadZipLocation();
QString getDownloadZipLocation();
QString getUnzippedLocation(int pathId);

bool validatePath(const QString path);
QString getBasePath(const QString path);
std::vector<FileHash> getHashFiles(const std::vector<QString>& filePaths,
                                   const QString& basePath);
std::vector<FileHash> createZipForUpload(const int pathId, const QString path);
bool unzipZipAtDownload(const int pathId);
std::vector<QString> listFiles(const QString basePath, const QString pattern);
QString extractPattern(const QString fullPath);
bool writeFileToFileSystemAtDownload(const int pathId, const QByteArray data);
bool replaceFileAtDestination(const int pathId, const QString destination);
}; // namespace utilFileSystem

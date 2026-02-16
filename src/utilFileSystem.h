#pragma once

#include <QString>

namespace utilFileSystem {
struct FileHash {
    QString relativePath;
    QString hash;

    auto operator==(const FileHash& other) const noexcept -> bool {
        return relativePath == other.relativePath && hash == other.hash;
    }

    auto operator!=(const FileHash& other) const noexcept -> bool { return !(*this == other); }
};

auto getUploadZipLocation() -> QString;
auto getDownloadZipLocation() -> QString;
auto getUnzippedLocation(int pathId) -> QString;

auto validatePath(const QString path) -> bool;
auto getBasePath(const QString path) -> QString;
auto getHashFiles(const std::vector<QString>& filePaths, const QString& basePath)
    -> std::vector<FileHash>;
auto createZipForUpload(const int pathId, const QString path) -> std::vector<FileHash>;
auto unzipZipAtDownload(const int pathId) -> bool;
auto listFiles(const QString basePath, const QString pattern) -> std::vector<QString>;
auto extractPattern(const QString fullPath) -> QString;
auto writeFileToFileSystemAtDownload(const int pathId, const QByteArray data) -> bool;
auto replaceFileAtDestination(const int pathId, const QString destination) -> bool;
}; // namespace utilFileSystem

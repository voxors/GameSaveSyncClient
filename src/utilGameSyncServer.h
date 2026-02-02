#pragma once

#include "error.h"
#include "utilFileSystem.h"
#include <QJsonDocument>
#include <QMap>
#include <QNetworkAccessManager>
#include <QUrl>
#include <expected>

class UtilGameSyncServer {
  public:
    struct GameMetadata {
        int id;
        QString defaultName;
        QString steamAppId;
        QList<QString> knownNames;
    };

    struct GameDefaultName {
        int id;
        QString defaultName;
    };

    struct GamePath {
        int id;
        QString operatingSystem;
        QString path;
    };

    struct SaveHash {
        QString hash;
        QString relativePath;
    };

    struct SaveJson {
        QList<SaveHash> savesHash;
        int pathId;
        qint64 unixTime;
        QString uuid;
    };

    struct ExecutableJson {
        int id;
        QString executablePath;
        QString operatingSystem;
    };

    struct GameSavesReturn {
        QString uuid;
        QByteArray data;
    };

    static constexpr QStringView windowsOS = u"windows";
    static constexpr QStringView linuxOS = u"linux";
    static constexpr QStringView undefined = u"undefined";

#if defined(Q_OS_WIN)
    QList<QString> listOfAcceptableOs = {QString(windowsOS), QString(undefined)};
#elif defined(Q_OS_LINUX)
    QList<QString> listOfAcceptableOs = {QString(windowsOS), QString(linuxOS), QString(undefined)};
#endif

    static UtilGameSyncServer& getInstance() {
        static UtilGameSyncServer instance;
        return instance;
    }

    std::expected<QList<UtilGameSyncServer::GameMetadata>, GameSaveSyncError::Error>
    getGameMetadataList(bool forceFetch = false);
    std::expected<QList<UtilGameSyncServer::GameDefaultName>, GameSaveSyncError::Error>
    getGameDefaultNameList(bool forceFetch = false);
    std::expected<UtilGameSyncServer::GameMetadata, GameSaveSyncError::Error>
    getGameMetadata(int gameId);
    std::expected<QList<UtilGameSyncServer::GamePath>, GameSaveSyncError::Error>
    getPathsByGameId(int gameId, bool forceFetch = false);
    std::expected<QList<UtilGameSyncServer::ExecutableJson>, GameSaveSyncError::Error>
    getExecutableByGameId(int id, bool forceFetch = false);
    std::expected<QList<UtilGameSyncServer::SaveJson>, GameSaveSyncError::Error>
    getSavesReferencesForPathId(int id);
    std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error>
    getGameSavesForPathId(int pathId);
    std::expected<QString, GameSaveSyncError::Error>
    postGameSavesForPathId(int pathId, std::vector<utilFileSystem::FileHash> hashOfContent);
    bool testConnection(QUrl testURL, QString apiToken);
    std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error>
    fetchLastSaveFromServer(int pathId);
    std::expected<void, GameSaveSyncError::Error> pushLocalSaveToServer(int pathId);
    std::expected<QString, GameSaveSyncError::Error> fetchDbUUID();
    UtilGameSyncServer(UtilGameSyncServer const&) = delete;
    UtilGameSyncServer& operator=(UtilGameSyncServer const&) = delete;

  protected:
    UtilGameSyncServer() = default;
    ~UtilGameSyncServer() = default;

  private:
    QList<GameMetadata> gameMetadataList;
    QList<GameDefaultName> gameDefaultNameList;
    QMap<int, QList<GamePath>> gamePathMap;
    QMap<int, QList<ExecutableJson>> gameExecutableMap;

    std::expected<QByteArray, GameSaveSyncError::Error>
    fetchRemoteEndpoint(QString endpoint, QUrl forcedURL = {}, QString forcedAPIToken = {},
                        bool validateUUID = true);
    std::expected<QJsonDocument, GameSaveSyncError::Error>
    fetchRemoteJSONEndpoint(QString endpoint, QUrl forcedURL = {}, QString forcedAPIToken = {},
                            bool validateUUID = true);
    std::expected<void, GameSaveSyncError::Error> validateDbUUID();
};

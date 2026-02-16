#pragma once

#include "error.h"
#include "utilFileSystem.h"
#include <QJsonDocument>
#include <QMap>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QUrlQuery>
#include <expected>

class UtilGameSyncServer {
  public:
    struct GameMetadata {
        int id;
        QString defaultName;
        QString steamAppId;
        QList<QString> knownNames;
        std::optional<QString> installDir;

        static auto gameMetadataFromJson(QJsonObject object)
            -> std::expected<UtilGameSyncServer::GameMetadata, GameSaveSyncError::Error>;
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

    static auto getInstance() -> UtilGameSyncServer& {
        static UtilGameSyncServer instance;
        return instance;
    }

    auto getGameMetadataList(bool forceFetch = false)
        -> std::expected<QList<UtilGameSyncServer::GameMetadata>, GameSaveSyncError::Error>;
    auto getGameDefaultNameList(bool forceFetch = false)
        -> std::expected<QList<UtilGameSyncServer::GameDefaultName>, GameSaveSyncError::Error>;
    auto getGameSearchDefaultNameList(const QString query)
        -> std::expected<QList<UtilGameSyncServer::GameDefaultName>, GameSaveSyncError::Error>;
    auto getGameMetadata(int gameId)
        -> std::expected<UtilGameSyncServer::GameMetadata, GameSaveSyncError::Error>;
    auto getPathsByGameId(int gameId, bool forceFetch = false)
        -> std::expected<QList<UtilGameSyncServer::GamePath>, GameSaveSyncError::Error>;
    auto getExecutableByGameId(int id, bool forceFetch = false)
        -> std::expected<QList<UtilGameSyncServer::ExecutableJson>, GameSaveSyncError::Error>;
    auto getSavesReferencesForPathId(int id)
        -> std::expected<QList<UtilGameSyncServer::SaveJson>, GameSaveSyncError::Error>;
    auto getGameSavesForPathId(int pathId)
        -> std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error>;
    auto postGameSavesForPathId(int pathId, std::vector<utilFileSystem::FileHash> hashOfContent)
        -> std::expected<QString, GameSaveSyncError::Error>;
    auto testConnection(QUrl testURL, QString apiToken) -> bool;
    auto fetchLastSaveFromServer(int pathId)
        -> std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error>;
    auto pushLocalSaveToServer(int pathId) -> std::expected<void, GameSaveSyncError::Error>;
    auto fetchDbUUID() -> std::expected<QString, GameSaveSyncError::Error>;
    UtilGameSyncServer(UtilGameSyncServer const&) = delete;
    auto operator=(UtilGameSyncServer const&) -> UtilGameSyncServer& = delete;

  protected:
    UtilGameSyncServer() = default;
    ~UtilGameSyncServer() = default;

  private:
    QList<GameMetadata> gameMetadataList;
    QList<GameDefaultName> gameDefaultNameList;
    QMap<int, QList<GamePath>> gamePathMap;
    QMap<int, QList<ExecutableJson>> gameExecutableMap;

    auto fetchRemoteEndpoint(QString endpoint, QUrlQuery query = {}, QUrl forcedURL = {},
                             QString forcedAPIToken = {}, bool validateUUID = true)
        -> std::expected<QByteArray, GameSaveSyncError::Error>;
    auto fetchRemoteJSONEndpoint(QString endpoint, QUrlQuery query = {}, QUrl forcedURL = {},
                                 QString forcedAPIToken = {}, bool validateUUID = true)
        -> std::expected<QJsonDocument, GameSaveSyncError::Error>;
    auto validateDbUUID() -> std::expected<void, GameSaveSyncError::Error>;
};

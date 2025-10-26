#pragma once

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
    QList<QString> listOfAcceptableOs = {QString(windowsOS),
                                         QString(undefined)};
#elif defined(Q_OS_LINUX)
    QList<QString> listOfAcceptableOs = {QString(windowsOS), QString(linuxOS),
                                         QString(undefined)};
#endif

    static UtilGameSyncServer& getInstance() {
        static UtilGameSyncServer instance;
        return instance;
    }

    QList<GameMetadata> getGameMetadataList(bool forceFetch = false);
    std::optional<UtilGameSyncServer::GameMetadata> getGameMetadata(int id);
    std::optional<QList<UtilGameSyncServer::GamePath>>
    getPathByGameId(int gameId, bool forceFetch = false);
    QList<ExecutableJson> getExecutableByGameId(int id,
                                                bool forceFetch = false);
    QList<SaveJson> getSavesReferencesForPathId(int id);
    std::expected<UtilGameSyncServer::GameSavesReturn, QString>
    getGameSavesForPathId(int pathId);
    std::optional<QString>
    postGameSavesForPathId(int pathId,
                           std::vector<utilFileSystem::FileHash> hashOfContent);
    bool testConnection(QUrl testURL);
    std::expected<UtilGameSyncServer::GameSavesReturn, QString>
    fetchLastSaveFromServer(int pathId);

    UtilGameSyncServer(UtilGameSyncServer const&) = delete;
    UtilGameSyncServer& operator=(UtilGameSyncServer const&) = delete;

  protected:
    UtilGameSyncServer() = default;
    ~UtilGameSyncServer() = default;

  private:
    QList<GameMetadata> gameMetadataList;
    QMap<int, QList<GamePath>> gamePathMap;
    QMap<int, QList<ExecutableJson>> gameExecutableMap;

    QByteArray fetchRemoteEndpoint(QString endpoint, QUrl forcedURL = {});
    QJsonDocument fetchRemoteJSONEndpoint(QString endpoint,
                                          QUrl forcedURL = {});
};

#include "utilGameSyncServer.h"
#include "config.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>

std::expected<QByteArray, GameSaveSyncError::Error>
UtilGameSyncServer::fetchRemoteEndpoint(QString endpoint, QUrlQuery query, QUrl forcedURL,
                                        QString forcedAPIToken, bool validateUUID) {
    if (validateUUID) {
        if (auto validUuid = validateDbUUID(); !validUuid)
            return std::unexpected{validUuid.error()};
    }
    QNetworkAccessManager manager;
    QUrl baseUrl = config::getRemoteURL().adjusted(QUrl::StripTrailingSlash);
    if (!forcedURL.isEmpty())
        baseUrl = forcedURL.adjusted(QUrl::StripTrailingSlash);
    QString apiToken = config::getAPIToken().trimmed();
    if (!forcedAPIToken.isEmpty())
        apiToken = forcedAPIToken.trimmed();
    baseUrl.setPath(baseUrl.path() + endpoint);
    baseUrl.setQuery(query);
    QNetworkRequest request(baseUrl);
    request.setRawHeader("Authorization", "Bearer " + apiToken.toUtf8());
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        switch (reply->error()) {
        case QNetworkReply::ContentNotFoundError:
            return std::unexpected{GameSaveSyncError::Error{
                .type = GameSaveSyncError::NotFound,
                .message = endpoint + " Error fetching endpoint:" + reply->errorString()}};
        default:
            return std::unexpected{GameSaveSyncError::Error{
                .type = GameSaveSyncError::Network,
                .message = endpoint + " Error fetching endpoint:" + reply->errorString()}};
        }
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return data;
}

std::expected<QJsonDocument, GameSaveSyncError::Error>
UtilGameSyncServer::fetchRemoteJSONEndpoint(QString endpoint, QUrlQuery query, QUrl forcedURL,
                                            QString forcedAPIToken, bool validateUUID) {
    auto data = fetchRemoteEndpoint(endpoint, query, forcedURL, forcedAPIToken, validateUUID);
    if (!data)
        return std::unexpected{data.error()};

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data.value(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return {};
    }

    return doc;
}

std::expected<QList<UtilGameSyncServer::GameMetadata>, GameSaveSyncError::Error>
UtilGameSyncServer::getGameMetadataList(bool forceFetch) {
    if (forceFetch || gameMetadataList.isEmpty()) {
        std::expected<QJsonDocument, GameSaveSyncError::Error> resultDocument =
            fetchRemoteJSONEndpoint("/v1/games");
        if (!resultDocument)
            return std::unexpected{resultDocument.error()};
        QList<UtilGameSyncServer::GameMetadata> gamesMetadata;

        QJsonArray outerArray = resultDocument.value().array();
        for (const QJsonValue& innerVal : outerArray) {
            QJsonObject object = innerVal.toObject();

            auto defaultNameJsonValue = object.value("default_name");
            if (defaultNameJsonValue.isNull()) {
                return std::unexpected{GameSaveSyncError::Error{
                    .type = GameSaveSyncError::Other,
                    .message = QString("Error while parsing default_name in ") + __FUNCTION__}};
            }
            QString defaultName = defaultNameJsonValue.toString();
            int id = object.value("id").toInt();
            QString steamAppId = object.value("steam_appid").toString();

            QList<QString> knowNames;
            for (auto knowName : object.value("known_name").toArray()) {
                knowNames.append(knowName.toString());
            }

            gamesMetadata.push_back({.id = id,
                                     .defaultName = defaultName,
                                     .steamAppId = steamAppId,
                                     .knownNames = knowNames});
        }
        gameMetadataList = gamesMetadata;
    }
    return gameMetadataList;
}

std::expected<QList<UtilGameSyncServer::GameDefaultName>, GameSaveSyncError::Error>
UtilGameSyncServer::getGameDefaultNameList(bool forceFetch) {
    if (forceFetch || gameDefaultNameList.isEmpty()) {
        std::expected<QJsonDocument, GameSaveSyncError::Error> resultDocument =
            fetchRemoteJSONEndpoint("/v1/games/default_name");
        if (!resultDocument)
            return std::unexpected{resultDocument.error()};
        QList<UtilGameSyncServer::GameDefaultName> gamesDefaultName;

        QJsonArray outerArray = resultDocument.value().array();
        for (const QJsonValue& innerVal : outerArray) {
            QJsonObject object = innerVal.toObject();

            auto defaultNameJsonValue = object.value("default_name");
            if (defaultNameJsonValue.isNull()) {
                return std::unexpected{GameSaveSyncError::Error{
                    .type = GameSaveSyncError::Other,
                    .message = QString("Error while parsing default_name in ") + __FUNCTION__}};
            }
            QString defaultName = defaultNameJsonValue.toString();
            int id = object.value("id").toInt();

            gamesDefaultName.push_back({
                .id = id,
                .defaultName = defaultName,
            });
        }
        gameDefaultNameList = gamesDefaultName;
    }
    return gameDefaultNameList;
}

std::expected<QList<UtilGameSyncServer::GameDefaultName>, GameSaveSyncError::Error>
UtilGameSyncServer::getGameSearchDefaultNameList(QString query) {
    std::expected<QJsonDocument, GameSaveSyncError::Error> resultDocument =
        fetchRemoteJSONEndpoint("/v1/games/search", QUrlQuery({std::pair("name", query)}));
    if (!resultDocument)
        return std::unexpected{resultDocument.error()};
    QList<UtilGameSyncServer::GameDefaultName> gamesDefaultName;

    QJsonArray outerArray = resultDocument.value().array();
    for (const QJsonValue& innerVal : outerArray) {
        QJsonObject object = innerVal.toObject();

        auto defaultNameJsonValue = object.value("default_name");
        if (defaultNameJsonValue.isNull()) {
            return std::unexpected{GameSaveSyncError::Error{
                .type = GameSaveSyncError::Other,
                .message = QString("Error while parsing default_name in ") + __FUNCTION__}};
        }
        QString defaultName = defaultNameJsonValue.toString();
        int id = object.value("id").toInt();

        gamesDefaultName.push_back({
            .id = id,
            .defaultName = defaultName,
        });
    }
    return gamesDefaultName;
}

std::expected<UtilGameSyncServer::GameMetadata, GameSaveSyncError::Error>
UtilGameSyncServer::getGameMetadata(int gameId) {
    QString endpoint = "/v1/games/" + QString::number(gameId);
    std::expected<QJsonDocument, GameSaveSyncError::Error> resultDocument =
        fetchRemoteJSONEndpoint(endpoint);
    if (!resultDocument)
        return std::unexpected{resultDocument.error()};

    QJsonDocument document = resultDocument.value();
    QJsonObject object = document.object();

    auto defaultNameJsonValue = object.value("default_name");
    if (defaultNameJsonValue.isNull()) {
        return std::unexpected{GameSaveSyncError::Error{
            .type = GameSaveSyncError::Other,
            .message = QString("Error while parsing default_name in ") + __FUNCTION__}};
    }
    QString defaultName = defaultNameJsonValue.toString();
    int id = object.value("id").toInt();
    QString steamAppId = object.value("steam_appid").toString();

    QList<QString> knowNames;
    for (auto knowName : object.value("known_name").toArray()) {
        knowNames.append(knowName.toString());
    }

    return GameMetadata{
        .id = id, .defaultName = defaultName, .steamAppId = steamAppId, .knownNames = knowNames};
}

std::expected<QList<UtilGameSyncServer::GamePath>, GameSaveSyncError::Error>
UtilGameSyncServer::getPathsByGameId(int gameId, bool forceFetch) {
    if (forceFetch || !this->gamePathMap.contains(gameId)) {
        QString endpoint = "/v1/games/" + QString::number(gameId) + "/paths";
        std::expected<QJsonDocument, GameSaveSyncError::Error> resultDocument =
            fetchRemoteJSONEndpoint(endpoint);
        if (!resultDocument)
            return std::unexpected{resultDocument.error()};

        QJsonDocument document = resultDocument.value();
        if (!document.isArray())
            return std::unexpected{
                GameSaveSyncError::Error{.type = GameSaveSyncError::Parsing,
                                         .message = QString("Parsing error in ") + __FUNCTION__}};
        QList<UtilGameSyncServer::GamePath> gamesPath;
        for (const QJsonValue& value : document.array()) {
            QJsonObject obj = value.toObject();
            int pathId = obj.value("id").toInt();
            QString path = obj.value("path").toString();
            QString operatingSystem = obj.value("operating_system").toString();

            if (!listOfAcceptableOs.contains(operatingSystem))
                continue;

            gamesPath.append({.id = pathId, .operatingSystem = operatingSystem, .path = path});
        }
        gamePathMap[gameId] = gamesPath;
    }
    return gamePathMap.value(gameId);
}

std::expected<QList<UtilGameSyncServer::ExecutableJson>, GameSaveSyncError::Error>
UtilGameSyncServer::getExecutableByGameId(int gameId, bool forceFetch) {
    if (forceFetch || !this->gameExecutableMap.contains(gameId)) {
        QString endpoint = "/v1/games/" + QString::number(gameId) + "/executables";
        std::expected<QJsonDocument, GameSaveSyncError::Error> resultDocument =
            fetchRemoteJSONEndpoint(endpoint);
        if (!resultDocument)
            return std::unexpected{resultDocument.error()};

        QJsonDocument document = resultDocument.value();
        if (!document.isArray())
            return std::unexpected{
                GameSaveSyncError::Error{.type = GameSaveSyncError::Parsing,
                                         .message = QString("Parsing error in ") + __FUNCTION__}};
        QJsonArray outerArray = document.array();
        QList<UtilGameSyncServer::ExecutableJson> executablesJson;
        for (const QJsonValue& objVal : outerArray) {
            if (!objVal.isObject()) {
                continue;
            }
            QJsonObject obj = objVal.toObject();
            int executableId = obj.value("id").toInt();
            QString executable = obj.value("executable").toString();
            QString operatingSystem = obj.value("operating_system").toString();

            if (!listOfAcceptableOs.contains(operatingSystem))
                continue;

            if (!executable.isEmpty()) {
                executablesJson.append({.id = executableId,
                                        .executablePath = executable,
                                        .operatingSystem = operatingSystem});
            }
        }
        gameExecutableMap[gameId] = executablesJson;
    }
    return gameExecutableMap.value(gameId);
}

std::expected<QList<UtilGameSyncServer::SaveJson>, GameSaveSyncError::Error>
UtilGameSyncServer::getSavesReferencesForPathId(int id) {
    QString endpoint = "/v1/paths/" + QString::number(id) + "/saves";
    std::expected<QJsonDocument, GameSaveSyncError::Error> resultDocument =
        fetchRemoteJSONEndpoint(endpoint);
    if (!resultDocument)
        return std::unexpected{resultDocument.error()};

    QJsonDocument document = resultDocument.value();
    if (!document.isArray())
        return std::unexpected{
            GameSaveSyncError::Error{.type = GameSaveSyncError::Parsing,
                                     .message = QString("Parsing error in ") + __FUNCTION__}};

    QList<UtilGameSyncServer::SaveJson> savesJson;

    for (auto element : document.array()) {
        if (element.isObject()) {
            QJsonObject object = element.toObject();
            QList<UtilGameSyncServer::SaveHash> savesHash;

            QJsonValue filesHash = object.value("files_hash");
            if (object.value("files_hash").isArray()) {
                for (auto fileHash : filesHash.toArray()) {
                    QJsonObject fileHashObject = fileHash.toObject();
                    savesHash.append(
                        {.hash = fileHashObject.value("hash").toString(),
                         .relativePath = fileHashObject.value("relative_path").toString()});
                }
            }

            savesJson.append({.savesHash = savesHash,
                              .pathId = object.value("path_id").toInt(),
                              .unixTime = object.value("time").toInteger(),
                              .uuid = object.value("uuid").toString()});
        }
    }

    return savesJson;
}

std::expected<QString, GameSaveSyncError::Error>
UtilGameSyncServer::postGameSavesForPathId(int pathId,
                                           std::vector<utilFileSystem::FileHash> hashOfContent) {
    QString endpoint = "/v1/paths/" + QString::number(pathId) + "/saves/upload";
    QUrl url = config::getRemoteURL().adjusted(QUrl::StripTrailingSlash);
    url.setPath(url.path() + endpoint);

    QJsonArray hashArray;
    for (const auto& fileHash : hashOfContent) {
        QJsonObject hashObj;
        hashObj["relative_path"] = fileHash.relativePath;
        hashObj["hash"] = fileHash.hash;
        hashArray.append(hashObj);
    }
    QJsonDocument hashDoc(hashArray);

    QString tempDir = utilFileSystem::getUploadZipLocation();
    QString zipPath = QDir(tempDir).filePath(QString::number(pathId) + ".zip");

    auto multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart httpPartHashArray;
    httpPartHashArray.setHeader(QNetworkRequest::ContentDispositionHeader,
                                QVariant("form-data; name=\"file_hash\""));
    httpPartHashArray.setBody(hashDoc.toJson(QJsonDocument::JsonFormat::Compact));

    QHttpPart httpPartFile;
    httpPartFile.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QVariant("form-data; name=\"file\""));
    auto file = new QFile(zipPath);
    if (!file->open(QIODevice::ReadOnly)) {
        return std::unexpected{GameSaveSyncError::Error{.type = GameSaveSyncError::Other,
                                                        .message = "Opening file failed"}};
    }
    httpPartFile.setBodyDevice(file);
    file->setParent(multiPart);

    multiPart->append(httpPartHashArray);
    multiPart->append(httpPartFile);

    QString apiToken = config::getAPIToken().trimmed();
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + apiToken.toUtf8());
    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.post(request, multiPart);
    multiPart->setParent(reply);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool success = (reply->error() == QNetworkReply::NoError);
    if (!success) {
        return std::unexpected{
            GameSaveSyncError::Error{.type = GameSaveSyncError::Network,
                                     .message = "Upload failed:" + reply->errorString()}};
    }

    QString uuid = QString::fromUtf8(reply->readAll());

    reply->deleteLater();

    return std::expected<QString, GameSaveSyncError::Error>{uuid};
}

std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error>
UtilGameSyncServer::getGameSavesForPathId(int pathId) {
    QByteArray save{};
    QString uuid;

    std::expected<QList<UtilGameSyncServer::SaveJson>, GameSaveSyncError::Error> resultSavesJson =
        UtilGameSyncServer::getInstance().getSavesReferencesForPathId(pathId);
    if (!resultSavesJson)
        return std::unexpected{resultSavesJson.error()};
    QList<SaveJson> savesJson = resultSavesJson.value();
    if (!savesJson.isEmpty()) {
        std::ranges::sort(savesJson,
                          [](const UtilGameSyncServer::SaveJson& value1,
                             const UtilGameSyncServer::SaveJson& value2) -> int {
                              return value1.unixTime < value2.unixTime;
                          });

        UtilGameSyncServer::SaveJson lastSave = savesJson.last();
        QString endpoint = "/v1/saves/" + lastSave.uuid;

        auto resultSave = fetchRemoteEndpoint(endpoint);
        if (!resultSave)
            return std::unexpected{resultSave.error()};
        save = resultSave.value();
        uuid = lastSave.uuid;
    }

    return !(save.isEmpty() || uuid.isEmpty())
               ? std::expected<UtilGameSyncServer::GameSavesReturn,
                               GameSaveSyncError::Error>{GameSavesReturn{.uuid = uuid,
                                                                         .data = save}}
               : std::unexpected{GameSaveSyncError::Error{
                     .type = GameSaveSyncError::Other,
                     .message = "Undefined error while downloading save file"}};
}

bool UtilGameSyncServer::testConnection(QUrl testURL, QString apiToken) {
    return testURL.isValid() &&
           fetchRemoteEndpoint("/v1/health", {}, testURL, apiToken, false).has_value();
}

std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error>
UtilGameSyncServer::fetchLastSaveFromServer(int pathId) {
    auto path = config::getPath(pathId);
    if (!utilFileSystem::validatePath(path)) {
        return std::unexpected{GameSaveSyncError::Error{.type = GameSaveSyncError::Other,
                                                        .message = "Invalid Path in Config"}};
    }
    return getGameSavesForPathId(pathId)
        .and_then(
            [&](UtilGameSyncServer::GameSavesReturn gameSave)
                -> std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error> {
                if (utilFileSystem::writeFileToFileSystemAtDownload(pathId, gameSave.data)) {
                    return std::expected<UtilGameSyncServer::GameSavesReturn,
                                         GameSaveSyncError::Error>{gameSave};
                } else {
                    return std::unexpected{GameSaveSyncError::Error{
                        .type = GameSaveSyncError::Other,
                        .message = "Failed to write tmp file to file system"}};
                }
            })
        .and_then(
            [&](UtilGameSyncServer::GameSavesReturn gameSave)
                -> std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error> {
                if (utilFileSystem::unzipZipAtDownload(pathId)) {
                    return std::expected<UtilGameSyncServer::GameSavesReturn,
                                         GameSaveSyncError::Error>{gameSave};
                } else {
                    return std::unexpected{GameSaveSyncError::Error{
                        .type = GameSaveSyncError::Other, .message = "Failed to unzip the file"}};
                }
            })
        .and_then(
            [&](UtilGameSyncServer::GameSavesReturn gameSave)
                -> std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error> {
                if (utilFileSystem::replaceFileAtDestination(pathId, config::getPath(pathId))) {
                    config::updateUUIDForPath(pathId, gameSave.uuid);
                    return std::expected<UtilGameSyncServer::GameSavesReturn,
                                         GameSaveSyncError::Error>{gameSave};
                } else {
                    return std::unexpected{GameSaveSyncError::Error{
                        .type = GameSaveSyncError::Other,
                        .message = "Failed to copy save content to destination"}};
                }
            })
        .or_else(
            [&](const GameSaveSyncError::Error& error)
                -> std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error> {
                return std::unexpected(error);
            });
}

std::expected<void, GameSaveSyncError::Error>
UtilGameSyncServer::pushLocalSaveToServer(int pathId) {
    auto path = config::getPath(pathId);
    if (!utilFileSystem::validatePath(path)) {
        return std::unexpected{GameSaveSyncError::Error{.type = GameSaveSyncError::Other,
                                                        .message = "Invalid Path in Config"}};
    }
    auto hashes = utilFileSystem::createZipForUpload(pathId, config::getPath(pathId));
    auto uuid = UtilGameSyncServer::getInstance().postGameSavesForPathId(pathId, hashes);
    if (!uuid) {
        return std::unexpected{GameSaveSyncError::Error{.type = GameSaveSyncError::Network,
                                                        .message = "failed to post game save"}};
    }
    config::updateUUIDForPath(pathId, uuid.value());
    return std::expected<void, GameSaveSyncError::Error>{};
}

std::expected<QString, GameSaveSyncError::Error> UtilGameSyncServer::fetchDbUUID() {
    std::expected<QByteArray, GameSaveSyncError::Error> result =
        fetchRemoteEndpoint("/v1/uuid", {}, {}, {}, false);
    if (!result)
        return std::unexpected(result.error());

    QByteArray data = result.value();
    QString uuid = QString::fromUtf8(data).trimmed();
    return uuid;
}

std::expected<void, GameSaveSyncError::Error> UtilGameSyncServer::validateDbUUID() {
    auto maybeUuid = fetchDbUUID();
    if (!maybeUuid)
        return std::unexpected{GameSaveSyncError::Error{
            .type = GameSaveSyncError::Network,
            .message = "Error while fetching the database uuid on the server"}};

    if (!config::validateDbUUID(maybeUuid.value())) {
        return std::unexpected{GameSaveSyncError::Error{
            .type = GameSaveSyncError::Other,
            .message = "The database uuid is not the same in the config as on the server"}};
    }
    return std::expected<void, GameSaveSyncError::Error>{};
}

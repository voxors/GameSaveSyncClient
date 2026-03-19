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

auto UtilGameSyncServer::GameMetadata::gameMetadataFromJson(QJsonObject object)
    -> std::expected<UtilGameSyncServer::GameMetadata, GameSaveSyncError::Error> {
    auto defaultNameJsonValue = object.value("default_name");
    if (defaultNameJsonValue.isNull()) {
        return std::unexpected{GameSaveSyncError::Error{
            .type = GameSaveSyncError::Other,
            .message = QString("Error while parsing default_name in ") + __FUNCTION__}};
    }
    QString defaultName = defaultNameJsonValue.toString();
    int id = object.value("id").toInt();
    QString steamAppId = object.value("steam_appid").toString();

    std::optional<QString> installDir = std::nullopt;
    if (object.contains("install_dir"))
        installDir = object.value("install_dir").toString();

    QList<QString> knowNames;
    for (auto knowName : object.value("known_name").toArray()) {
        knowNames.append(knowName.toString());
    }

    return UtilGameSyncServer::GameMetadata{.id = id,
                                            .defaultName = defaultName,
                                            .steamAppId = steamAppId,
                                            .knownNames = knowNames,
                                            .installDir = installDir};
}

auto UtilGameSyncServer::fetchRemoteEndpoint(QString endpoint, QUrlQuery query, QUrl forcedURL,
                                             QString forcedAPIToken, bool validateUUID)
    -> std::expected<QByteArray, GameSaveSyncError::Error> {
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

auto UtilGameSyncServer::fetchRemoteJSONEndpoint(QString endpoint, QUrlQuery query, QUrl forcedURL,
                                                 QString forcedAPIToken, bool validateUUID)
    -> std::expected<QJsonDocument, GameSaveSyncError::Error> {
    auto data = fetchRemoteEndpoint(endpoint, query, forcedURL, forcedAPIToken, validateUUID);
    if (!data)
        return std::unexpected{data.error()};

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data.value(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return std::unexpected{GameSaveSyncError::Error{
            .type = GameSaveSyncError::Parsing,
            .message = endpoint + " Error parsing Json:" + parseError.errorString()}};
    };

    return doc;
}

auto UtilGameSyncServer::getGameMetadataList(bool forceFetch)
    -> std::expected<QList<UtilGameSyncServer::GameMetadata>, GameSaveSyncError::Error> {
    if (forceFetch || gameMetadataList.isEmpty()) {
        std::expected<QJsonDocument, GameSaveSyncError::Error> resultDocument =
            fetchRemoteJSONEndpoint("/v1/games");
        if (!resultDocument)
            return std::unexpected{resultDocument.error()};
        QList<UtilGameSyncServer::GameMetadata> gamesMetadata;

        QJsonArray outerArray = resultDocument.value().array();
        for (const QJsonValue& innerVal : outerArray) {
            QJsonObject object = innerVal.toObject();
            auto maybeGameMetadata = GameMetadata::gameMetadataFromJson(object);
            if (maybeGameMetadata.has_value())
                gamesMetadata.push_back(maybeGameMetadata.value());
            else
                return std::unexpected<GameSaveSyncError::Error>{maybeGameMetadata.error()};
        }
        gameMetadataList = gamesMetadata;
    }
    return gameMetadataList;
}

auto UtilGameSyncServer::getGameDefaultNameList(bool forceFetch)
    -> std::expected<QList<UtilGameSyncServer::GameDefaultName>, GameSaveSyncError::Error> {
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

auto UtilGameSyncServer::getGameSearchDefaultNameList(QString query)
    -> std::expected<QList<UtilGameSyncServer::GameDefaultName>, GameSaveSyncError::Error> {
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

auto UtilGameSyncServer::getGameMetadata(int gameId)
    -> std::expected<UtilGameSyncServer::GameMetadata, GameSaveSyncError::Error> {
    QString endpoint = "/v1/games/" + QString::number(gameId);
    std::expected<QJsonDocument, GameSaveSyncError::Error> resultDocument =
        fetchRemoteJSONEndpoint(endpoint);
    if (!resultDocument)
        return std::unexpected{resultDocument.error()};

    QJsonDocument document = resultDocument.value();
    QJsonObject object = document.object();
    auto maybeGameMetadata = GameMetadata::gameMetadataFromJson(object);
    if (maybeGameMetadata.has_value())
        return maybeGameMetadata.value();
    else
        return std::unexpected<GameSaveSyncError::Error>{maybeGameMetadata.error()};
}

auto UtilGameSyncServer::getPathsByGameId(int gameId, bool forceFetch)
    -> std::expected<QList<UtilGameSyncServer::GamePath>, GameSaveSyncError::Error> {
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

auto UtilGameSyncServer::getExecutableByGameId(int gameId, bool forceFetch)
    -> std::expected<QList<UtilGameSyncServer::ExecutableJson>, GameSaveSyncError::Error> {
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

auto UtilGameSyncServer::getSavesReferencesForPathId(int id)
    -> std::expected<QList<UtilGameSyncServer::SaveJson>, GameSaveSyncError::Error> {
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

auto UtilGameSyncServer::postGameSavesForPathId(int pathId,
                                                std::vector<utilFileSystem::FileHash> hashOfContent)
    -> std::expected<QString, GameSaveSyncError::Error> {
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
    QByteArray hashJson = hashDoc.toJson();

    QString tempDir = utilFileSystem::getUploadZipLocation();
    QString zipPath = QDir(tempDir).filePath(QString::number(pathId) + ".zip");

    QFile zipFile(zipPath);
    if (!zipFile.open(QIODevice::ReadOnly)) {
        return std::unexpected(GameSaveSyncError::Error{.type = GameSaveSyncError::Other,
                                                        .message = "Failed to open zip file"});
    }
    QFileInfo zipFileInfo(zipFile);

    const qint64 chunkSize = 10L * 1024 * 1024;
    qint64 totalSize = zipFileInfo.size();
    qint64 totalChunks = (totalSize + chunkSize - 1) / chunkSize;

    QString uuid;

    QNetworkAccessManager manager;
    for (int chunkIdx = 0; chunkIdx < totalChunks; ++chunkIdx) {
        qint64 start = chunkIdx * chunkSize;

        zipFile.seek(start);
        QByteArray chunkBytes = zipFile.read(chunkSize);

        auto* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        if (!uuid.isEmpty()) {
            QHttpPart uuidPart;
            uuidPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                               QVariant(R"(form-data; name="uuid")"));
            uuidPart.setBody(uuid.toUtf8());
            multipart->append(uuidPart);
        }

        QHttpPart chunkNumberPart;
        chunkNumberPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                                  QVariant(R"(form-data; name="chunkNumber")"));
        chunkNumberPart.setBody(QString::number(chunkIdx).toUtf8());
        multipart->append(chunkNumberPart);

        QHttpPart totalChunksPart;
        totalChunksPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                                  QVariant(R"(form-data; name="totalChunks")"));
        totalChunksPart.setBody(QString::number(totalChunks).toUtf8());
        multipart->append(totalChunksPart);

        QHttpPart chunkPart;
        chunkPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                            QVariant(R"(form-data; name="chunk")"));
        chunkPart.setHeader(QNetworkRequest::ContentTypeHeader,
                            QVariant("application/octet-stream"));
        chunkPart.setBody(chunkBytes);
        multipart->append(chunkPart);

        QHttpPart hashPart;
        hashPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QVariant(R"(form-data; name="file_hash")"));
        hashPart.setBody(hashJson);
        multipart->append(hashPart);

        QString apiToken = config::getAPIToken().trimmed();
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", "Bearer " + apiToken.toUtf8());

        QEventLoop loop;
        QNetworkReply* reply = manager.post(request, multipart);
        multipart->setParent(reply);

        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            return std::unexpected(GameSaveSyncError::Error{.type = GameSaveSyncError::Network,
                                                            .message = reply->errorString()});
        }

        QByteArray response = reply->readAll();
        reply->deleteLater();

        QJsonDocument jsonResp = QJsonDocument::fromJson(response);
        if (!jsonResp.isObject()) {
            return std::unexpected(GameSaveSyncError::Error{.type = GameSaveSyncError::Other,
                                                            .message = "Invalid JSON response"});
        }
        QString respUuid = jsonResp.object().value("uuid").toString();
        if (respUuid.isEmpty()) {
            return std::unexpected(GameSaveSyncError::Error{.type = GameSaveSyncError::Other,
                                                            .message = "Missing uuid in response"});
        }
        uuid = respUuid;
    }

    zipFile.close();

    return uuid;
}

auto UtilGameSyncServer::getGameSavesForPathId(int pathId)
    -> std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error> {
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

auto UtilGameSyncServer::testConnection(QUrl testURL, QString apiToken) -> bool {
    return testURL.isValid() &&
           fetchRemoteEndpoint("/v1/health", {}, testURL, apiToken, false).has_value();
}

auto UtilGameSyncServer::fetchLastSaveFromServer(int pathId)
    -> std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error> {
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

auto UtilGameSyncServer::pushLocalSaveToServer(int pathId)
    -> std::expected<void, GameSaveSyncError::Error> {
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

auto UtilGameSyncServer::fetchDbUUID() -> std::expected<QString, GameSaveSyncError::Error> {
    std::expected<QByteArray, GameSaveSyncError::Error> result =
        fetchRemoteEndpoint("/v1/uuid", {}, {}, {}, false);
    if (!result)
        return std::unexpected(result.error());

    QByteArray data = result.value();
    QString uuid = QString::fromUtf8(data).trimmed();
    return uuid;
}

auto UtilGameSyncServer::validateDbUUID() -> std::expected<void, GameSaveSyncError::Error> {
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

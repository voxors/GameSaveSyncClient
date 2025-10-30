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

QByteArray UtilGameSyncServer::fetchRemoteEndpoint(QString endpoint, QUrl forcedURL) {
    QNetworkAccessManager manager;
    QUrl baseUrl = config::getRemoteURL().adjusted(QUrl::StripTrailingSlash);
    if (!forcedURL.isEmpty())
        baseUrl = forcedURL.adjusted(QUrl::StripTrailingSlash);
    baseUrl.setPath(baseUrl.path() + endpoint);
    QNetworkRequest request(baseUrl);
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Error fetching endpoint:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return data;
}

QJsonDocument UtilGameSyncServer::fetchRemoteJSONEndpoint(QString endpoint, QUrl forcedURL) {
    auto data = fetchRemoteEndpoint(endpoint, forcedURL);

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return {};
    }

    return doc;
}

QList<UtilGameSyncServer::GameMetadata> UtilGameSyncServer::getGameMetadataList(bool forceFetch) {
    if (forceFetch || gameMetadataList.isEmpty()) {
        QJsonDocument document = fetchRemoteJSONEndpoint("/v1/games");
        QList<UtilGameSyncServer::GameMetadata> gamesMetadata;

        QJsonArray outerArray = document.array();
        for (const QJsonValue& innerVal : outerArray) {
            QJsonObject object = innerVal.toObject();

            QString defaultName = object.value("default_name").toString();
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

std::optional<UtilGameSyncServer::GameMetadata> UtilGameSyncServer::getGameMetadata(int gameID) {
    QList<UtilGameSyncServer::GameMetadata> gameMetadataList = getGameMetadataList();
    for (const UtilGameSyncServer::GameMetadata& gameMetadata : gameMetadataList) {
        if (gameID == gameMetadata.id) {
            return gameMetadata;
        }
    }
    return std::nullopt;
}

std::optional<QList<UtilGameSyncServer::GamePath>>
UtilGameSyncServer::getPathByGameId(int gameId, bool forceFetch) {
    if (forceFetch || !this->gamePathMap.contains(gameId)) {
        QString endpoint = "/v1/games/" + QString::number(gameId) + "/paths";
        QJsonDocument document = fetchRemoteJSONEndpoint(endpoint);

        if (!document.isArray())
            return std::nullopt;
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

QList<UtilGameSyncServer::ExecutableJson>
UtilGameSyncServer::getExecutableByGameId(int gameId, bool forceFetch) {
    if (forceFetch || !this->gameExecutableMap.contains(gameId)) {
        QString endpoint = "/v1/games/" + QString::number(gameId) + "/executables";
        QJsonDocument document = fetchRemoteJSONEndpoint(endpoint);
        if (document.isArray()) {
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
    }
    return gameExecutableMap.value(gameId);
}

QList<UtilGameSyncServer::SaveJson> UtilGameSyncServer::getSavesReferencesForPathId(int id) {
    QString endpoint = "/v1/paths/" + QString::number(id) + "/saves";
    QJsonDocument document = fetchRemoteJSONEndpoint(endpoint);

    QList<UtilGameSyncServer::SaveJson> savesJson;

    if (document.isArray()) {
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
    }

    return savesJson;
}

std::optional<QString>
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
    qDebug() << "Zip path:" << zipPath;

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
        qWarning() << "Opening file failed";
        return std::nullopt;
    }
    httpPartFile.setBodyDevice(file);
    file->setParent(multiPart);

    multiPart->append(httpPartHashArray);
    multiPart->append(httpPartFile);

    QNetworkRequest request(url);
    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.post(request, multiPart);
    multiPart->setParent(reply);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool success = (reply->error() == QNetworkReply::NoError);
    if (!success) {
        qWarning() << "Upload failed:" << reply->errorString();
        return std::nullopt;
    }

    QString uuid = QString::fromUtf8(reply->readAll());

    reply->deleteLater();

    return uuid;
}

std::expected<UtilGameSyncServer::GameSavesReturn, QString>
UtilGameSyncServer::getGameSavesForPathId(int pathId) {
    QByteArray save{};
    QString uuid;

    QList<UtilGameSyncServer::SaveJson> savesJson =
        UtilGameSyncServer::getInstance().getSavesReferencesForPathId(pathId);
    if (!savesJson.isEmpty()) {
        std::ranges::sort(savesJson,
                          [](const UtilGameSyncServer::SaveJson& value1,
                             const UtilGameSyncServer::SaveJson& value2) -> int {
                              return value1.unixTime < value2.unixTime;
                          });

        UtilGameSyncServer::SaveJson lastSave = savesJson.last();
        QString endpoint = "/v1/saves/" + lastSave.uuid;

        save = fetchRemoteEndpoint(endpoint);
        uuid = lastSave.uuid;
    }

    return !save.isEmpty() || !uuid.isEmpty()
               ? std::expected<UtilGameSyncServer::GameSavesReturn, QString>{GameSavesReturn{
                     .uuid = uuid, .data = save}}
               : std::unexpected{"Undefined error while downloading save file"};
}

bool UtilGameSyncServer::testConnection(QUrl testURL) {
    return testURL.isValid() && !fetchRemoteJSONEndpoint("/v1/games", testURL).isEmpty();
}

std::expected<UtilGameSyncServer::GameSavesReturn, QString>
UtilGameSyncServer::fetchLastSaveFromServer(int pathId) {
    return getGameSavesForPathId(pathId)
        .and_then([&](UtilGameSyncServer::GameSavesReturn gameSave)
                      -> std::expected<UtilGameSyncServer::GameSavesReturn, QString> {
            if (utilFileSystem::writeFileToFileSystemAtDownload(pathId, gameSave.data)) {
                return std::expected<UtilGameSyncServer::GameSavesReturn, QString>{};
            } else {
                return std::unexpected{"Failed to write tmp file to file system"};
            }
        })
        .and_then([&](UtilGameSyncServer::GameSavesReturn gameSave)
                      -> std::expected<UtilGameSyncServer::GameSavesReturn, QString> {
            if (utilFileSystem::unzipZipAtDownload(pathId)) {
                return std::expected<UtilGameSyncServer::GameSavesReturn, QString>{gameSave};
            } else {
                return std::unexpected{"Failed to unzip the file"};
            }
        })
        .and_then([&](UtilGameSyncServer::GameSavesReturn gameSave)
                      -> std::expected<UtilGameSyncServer::GameSavesReturn, QString> {
            if (utilFileSystem::replaceFileAtDestination(pathId, config::getPath(pathId))) {
                config::updateUUIDForPath(pathId, gameSave.uuid);
                return std::expected<UtilGameSyncServer::GameSavesReturn, QString>{gameSave};
            } else {
                return std::unexpected{"Failed to copy save content to destination"};
            }
        })
        .or_else([&](const QString& error)
                     -> std::expected<UtilGameSyncServer::GameSavesReturn, QString> {
            qWarning() << "Path" << pathId << "error:" << error;
            return std::unexpected(error);
        });
}

std::expected<void, QString> UtilGameSyncServer::pushLocalSaveToServer(int pathId) {
    auto hashes = utilFileSystem::createZipForUpload(pathId, config::getPath(pathId));
    auto uuid = UtilGameSyncServer::getInstance().postGameSavesForPathId(pathId, hashes);
    if (!uuid.has_value())
        return std::unexpected<QString>{"failed to post game save"};
    config::updateUUIDForPath(pathId, uuid.value());
    return std::expected<void, QString>{};
}

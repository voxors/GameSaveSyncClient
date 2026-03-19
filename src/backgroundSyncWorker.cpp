#include "backgroundSyncWorker.h"

#include "config.h"
#include "status.h"
#include "utilFileSystem.h"
#include "utilGameSyncServer.h"
#include <algorithm>

constexpr int timerInterval = 30 * 1000;
constexpr qint64 savesMinimumInterval = static_cast<qint64>(5 * 60);

BackgroundSyncWorker::BackgroundSyncWorker(QObject* parent) : QObject(parent) {
    backgroundTimer = new QTimer(this);
    connect(backgroundTimer, &QTimer::timeout, this, &BackgroundSyncWorker::update);
}

void BackgroundSyncWorker::start() {
    QMetaObject::invokeMethod(
        backgroundTimer,
        [&]() -> void {
            update();
            backgroundTimer->setInterval(timerInterval);
            backgroundTimer->start();
        },
        Qt::QueuedConnection);
}

auto shouldUploadLocalFile(int pathId) -> std::expected<bool, GameSaveSyncError::Error> {
    QString configPath = config::getPath(pathId);
    if (!utilFileSystem::validatePath(configPath))
        return false;

    std::expected<QList<UtilGameSyncServer::SaveJson>, GameSaveSyncError::Error> resultSavesJson =
        UtilGameSyncServer::getInstance().getSavesReferencesForPathId(pathId);
    if (!resultSavesJson)
        return std::unexpected{resultSavesJson.error()};
    QList<UtilGameSyncServer::SaveJson> savesJson = resultSavesJson.value();
    if (savesJson.isEmpty())
        return true;

    std::ranges::sort(savesJson,
                      [](const UtilGameSyncServer::SaveJson& value1,
                         const UtilGameSyncServer::SaveJson& value2) -> int {
                          return value1.unixTime < value2.unixTime;
                      });

    UtilGameSyncServer::SaveJson lastSave = savesJson.last();
    // If I was not the one to have created the last save. I don't want to
    // continue. Need to be changed for a real conflict gui/manager at one point
    if (lastSave.uuid != config::getUUIDForPath(pathId)) {
        return false;
    }

    if ((QDateTime::currentSecsSinceEpoch() - lastSave.unixTime) < savesMinimumInterval) {
        return false;
    }

    std::vector<utilFileSystem::FileHash> dbHashes;
    dbHashes.reserve(lastSave.savesHash.size());
    std::ranges::transform(
        lastSave.savesHash, std::back_inserter(dbHashes),
        [](const UtilGameSyncServer::SaveHash& saveHash) -> utilFileSystem::FileHash {
            return utilFileSystem::FileHash{
                .relativePath = saveHash.relativePath,
                .hash = saveHash.hash,
            };
        });

    QString basePath = utilFileSystem::getBasePath(configPath);
    QString pattern = utilFileSystem::extractPattern(configPath);
    std::vector<QString> listOfFile = utilFileSystem::listFiles(basePath, pattern);
    std::vector<utilFileSystem::FileHash> currentFileHash =
        utilFileSystem::getHashFiles(listOfFile, basePath);

    if (dbHashes == currentFileHash) {
        return false;
    }

    return true;
}

auto forEachGamePath(int gameId, std::function<std::expected<void, GameSaveSyncError::Error>(
                                     int pathId, const QString& configPath)>
                                     callback) -> std::expected<void, GameSaveSyncError::Error> {
    auto& server = UtilGameSyncServer::getInstance();
    auto maybePaths = server.getPathsByGameId(gameId);
    if (!maybePaths)
        return std::unexpected{maybePaths.error()};
    for (const UtilGameSyncServer::GamePath& path : maybePaths.value()) {
        int pathId = path.id;
        QString configPath = config::getPath(pathId);
        auto result = callback(pathId, configPath);
        if (!result)
            return std::unexpected{result.error()};
    }
    return std::expected<void, GameSaveSyncError::Error>{};
}

auto BackgroundSyncWorker::syncGameSaveToServer() -> std::expected<void, GameSaveSyncError::Error> {
    for (int gameId : config::returnAllIds()) {
        auto result = forEachGamePath(
            gameId,
            [&](int pathId, const QString&) -> std::expected<void, GameSaveSyncError::Error> {
                QMutexLocker locker(&Status::getInstance().getLockedPathIdMutex(pathId));

                auto maybeShouldUpload = shouldUploadLocalFile(pathId);
                if (!maybeShouldUpload)
                    return std::unexpected<GameSaveSyncError::Error>{maybeShouldUpload.error()};
                if (!maybeShouldUpload.value())
                    return std::expected<void, GameSaveSyncError::Error>{};

                std::expected<void, GameSaveSyncError::Error> result =
                    UtilGameSyncServer::getInstance().pushLocalSaveToServer(pathId);
                if (!result)
                    return std::unexpected{result.error()};
                return std::expected<void, GameSaveSyncError::Error>{};
            });
        if (!result)
            return std::unexpected{result.error()};
    }
    return std::expected<void, GameSaveSyncError::Error>{};
}

auto shouldDownloadToLocalFile(int pathId) -> std::expected<bool, GameSaveSyncError::Error> {
    QString configPath = config::getPath(pathId);
    if (!utilFileSystem::validatePath(configPath))
        return false;

    std::expected<QList<UtilGameSyncServer::SaveJson>, GameSaveSyncError::Error> resultSavesJson =
        UtilGameSyncServer::getInstance().getSavesReferencesForPathId(pathId);
    if (!resultSavesJson)
        return std::unexpected{resultSavesJson.error()};
    QList<UtilGameSyncServer::SaveJson> savesJson = resultSavesJson.value();
    if (savesJson.isEmpty())
        return false;

    QString currentUUID = config::getUUIDForPath(pathId);

    std::ranges::sort(savesJson,
                      [](const UtilGameSyncServer::SaveJson& value1,
                         const UtilGameSyncServer::SaveJson& value2) -> int {
                          return value1.unixTime < value2.unixTime;
                      });

    UtilGameSyncServer::SaveJson lastSave = savesJson.last();
    if (lastSave.uuid == config::getUUIDForPath(pathId)) {
        return false;
    }

    auto currentSave =
        std::ranges::find_if(savesJson, [&](const UtilGameSyncServer::SaveJson& value) -> bool {
            return value.uuid == currentUUID;
        });
    if (currentSave == savesJson.end())
        return false;

    std::vector<utilFileSystem::FileHash> dbHashes;
    dbHashes.reserve(currentSave->savesHash.size());
    std::ranges::transform(
        currentSave->savesHash, std::back_inserter(dbHashes),
        [](const UtilGameSyncServer::SaveHash& saveHash) -> utilFileSystem::FileHash {
            return utilFileSystem::FileHash{
                .relativePath = saveHash.relativePath,
                .hash = saveHash.hash,
            };
        });

    QString basePath = utilFileSystem::getBasePath(configPath);
    QString pattern = utilFileSystem::extractPattern(configPath);
    std::vector<QString> listOfFile = utilFileSystem::listFiles(basePath, pattern);
    std::vector<utilFileSystem::FileHash> currentFileHash =
        utilFileSystem::getHashFiles(listOfFile, basePath);

    if (dbHashes != currentFileHash) {
        return false;
    }

    return true;
}

auto BackgroundSyncWorker::syncGameSaveFromServer()
    -> std::expected<void, GameSaveSyncError::Error> {
    for (int gameID : config::returnAllIds()) {
        std::expected<void, GameSaveSyncError::Error> result = forEachGamePath(
            gameID,
            [&](int pathId, const QString&) -> std::expected<void, GameSaveSyncError::Error> {
                QMutexLocker locker(&Status::getInstance().getLockedPathIdMutex(pathId));

                auto maybeShouldDownload = shouldDownloadToLocalFile(pathId);
                if (!maybeShouldDownload)
                    return std::unexpected<GameSaveSyncError::Error>{maybeShouldDownload.error()};
                if (!maybeShouldDownload.value())
                    return std::expected<void, GameSaveSyncError::Error>{};

                std::expected<UtilGameSyncServer::GameSavesReturn, GameSaveSyncError::Error>
                    result = UtilGameSyncServer::getInstance().fetchLastSaveFromServer(pathId);
                if (!result.has_value()) {
                    return std::unexpected{result.error()};
                }
                return std::expected<void, GameSaveSyncError::Error>{};
            });
        if (!result)
            return std::unexpected{result.error()};
    }
    return std::expected<void, GameSaveSyncError::Error>{};
}

void BackgroundSyncWorker::validatePaths() {
    QMap<int, QString> pathStatus;
    for (int gameId : config::returnAllIds()) {
        auto _ = forEachGamePath(gameId,
                                 [&](int pathId, const QString& configPath)
                                     -> std::expected<void, GameSaveSyncError::Error> {
                                     if (!utilFileSystem::validatePath(configPath))
                                         pathStatus.insert(pathId, {"Invalid Path in Config"});
                                     else
                                         pathStatus.insert(pathId, {});
                                     return std::expected<void, GameSaveSyncError::Error>{};
                                 });
    }
    Status::getInstance().setPathStatus(pathStatus);
}

void BackgroundSyncWorker::update() {
    try {
        validatePaths();
        auto result = syncGameSaveFromServer();
        if (!result) {
            emit errorOccurred(result.error());
        }
        result = syncGameSaveToServer();
        if (!result) {
            emit errorOccurred(result.error());
        }
        emit syncFinished();
    } catch (const std::exception& e) {
        emit errorOccurred(GameSaveSyncError::Error{.type = GameSaveSyncError::Other,
                                                    .message = QString::fromStdString(e.what())});
    }
}

void BackgroundSyncWorker::stop() {
    QMetaObject::invokeMethod(
        backgroundTimer, [&]() -> void { this->backgroundTimer->stop(); }, Qt::QueuedConnection);
}

auto BackgroundSyncWorker::isRunning() -> bool {
    if (backgroundTimer != nullptr) {
        return backgroundTimer->isActive();
    } else {
        return false;
    }
}

#include "backgroundSyncWorker.h"

#include "config.h"
#include "status.h"
#include "utilFileSystem.h"
#include "utilGameSyncServer.h"
#include <algorithm>

constexpr int timerInterval = 30 * 1000;
constexpr qint64 savesMinimumInterval = static_cast<qint64>(5 * 60);

void BackgroundSyncWorker::start() {
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BackgroundSyncWorker::update);
    update();
    timer->setInterval(timerInterval);
    timer->start();
}

bool shouldUploadLocalFile(int pathId) {
    QString configPath = config::getPath(pathId);
    if (!utilFileSystem::validatePath(configPath))
        return false;

    QList<UtilGameSyncServer::SaveJson> savesJson =
        UtilGameSyncServer::getInstance().getSavesReferencesForPathId(pathId);
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

    if ((QDateTime::currentSecsSinceEpoch() - lastSave.unixTime) <
        savesMinimumInterval) {
        return false;
    }

    std::vector<utilFileSystem::FileHash> dbHashes;
    dbHashes.reserve(lastSave.savesHash.size());
    std::ranges::transform(lastSave.savesHash, std::back_inserter(dbHashes),
                           [](const UtilGameSyncServer::SaveHash& saveHash)
                               -> utilFileSystem::FileHash {
                               return utilFileSystem::FileHash{
                                   .relativePath = saveHash.relativePath,
                                   .hash = saveHash.hash,
                               };
                           });

    QString basePath = utilFileSystem::getBasePath(configPath);
    QString pattern = utilFileSystem::extractPattern(configPath);
    std::vector<QString> listOfFile =
        utilFileSystem::listFiles(basePath, pattern);
    std::vector<utilFileSystem::FileHash> currentFileHash =
        utilFileSystem::getHashFiles(listOfFile, basePath);

    if (dbHashes == currentFileHash) {
        return false;
    }

    return true;
}

namespace {

void forEachGamePath(
    int gameId,
    std::function<void(int pathId, const QString& configPath)> callback) {
    auto& server = UtilGameSyncServer::getInstance();
    auto maybePaths = server.getPathByGameId(gameId);
    if (!maybePaths.has_value())
        return;
    for (const UtilGameSyncServer::GamePath& path : maybePaths.value()) {
        int pathId = path.id;
        QString configPath = config::getPath(pathId);
        callback(pathId, configPath);
    }
}

} // namespace

void BackgroundSyncWorker::syncGameSaveToServer() {
    for (int gameId : config::returnAllIds()) {
        forEachGamePath(gameId, [&](int pathId, const QString&) -> void {
            QMutexLocker locker(
                &Status::getInstance().getLockedPathIdMutex(pathId));

            if (!shouldUploadLocalFile(pathId))
                return;

            std::expected<void, QString> result =
                UtilGameSyncServer::getInstance().pushLocalSaveToServer(pathId);
            if (!result.has_value())
                qWarning() << "Error while sync Game Save to Server : " +
                                  result.error();
        });
    }
}

void BackgroundSyncWorker::validatePaths() {
    QMap<int, QString> pathStatus;
    for (int gameId : config::returnAllIds()) {
        forEachGamePath(gameId,
                        [&](int pathId, const QString& configPath) -> void {
                            if (!utilFileSystem::validatePath(configPath))
                                pathStatus.insert(pathId, {"Invalid Path"});
                            else
                                pathStatus.insert(pathId, {});
                        });
    }
    Status::getInstance().setPathStatus(pathStatus);
}

void BackgroundSyncWorker::update() {
    try {
        validatePaths();
        syncGameSaveToServer();
        emit syncFinished();
    } catch (const std::exception& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

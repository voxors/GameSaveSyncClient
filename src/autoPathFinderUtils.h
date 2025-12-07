#pragma once

#include "utilGameSyncServer.h"
#include <QMap>
#include <QString>

namespace AutoPathFinder {

enum PathTag {
    root,
    game,
    base,
    home,
    storeGameId,
    storeUserId,
    osUserName,
    winAppData,
    winLocalAppData,
    winLocalAppDataLow,
    winDocuments,
    winPublic,
    winProgramData,
    winDir,
    xdgData,
    xdgConfig,
};

QString getAutoPath(const UtilGameSyncServer::GameMetadata gameMetadata,
                    const UtilGameSyncServer::GamePath gamePath);

} // namespace AutoPathFinder

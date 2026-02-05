#include "autoPathFinderUtils.h"
#include "utilFileSystem.h"
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QRegularExpression>
#ifdef Q_OS_WINDOWS
#include <QSettings>
#endif

namespace AutoPathFinder {

static const QMap<QString, PathTag> tagListMap = {
    {"<root>", root},
    {"<game>", game},
    {"<base>", base},
    {"<home>", home},
    {"<storeGameId>", storeGameId},
    {"<storeUserId>", storeUserId},
    {"<osUserName>", osUserName},
    {"<winAppData>", winAppData},
    {"<winLocalAppData>", winLocalAppData},
    {"<winLocalAppDataLow>", winLocalAppDataLow},
    {"<winDocuments>", winDocuments},
    {"<winPublic>", winPublic},
    {"<winProgramData>", winProgramData},
    {"<winDir>", winDir},
    {"<xdgData>", xdgData},
    {"<xdgConfig>", xdgConfig},
};

std::optional<QString> getSteamConfigFolder() {
#ifdef Q_OS_LINUX
    return QDir::homePath() + "/.local/share/Steam/config/libraryfolders.vdf";
#elif defined(Q_OS_WINDOWS)
    QSettings reg(QStringLiteral("HKEY_CURRENT_USER\\Software\\Valve\\Steam"),
                  QSettings::NativeFormat);
    return QDir(reg.value(QStringLiteral("SteamPath")).toString())
        .absoluteFilePath("config/libraryfolders.vdf");
#endif
}

std::optional<QString> getGameLibraryFolder(const UtilGameSyncServer::GameMetadata gameMetadata) {
    if (gameMetadata.steamAppId.isEmpty()) {
        return std::nullopt;
    }

    auto maybeSteamConfigPath = getSteamConfigFolder();
    if (!maybeSteamConfigPath.has_value()) {
        return std::nullopt;
    }

    QFile file(maybeSteamConfigPath.value());
    if (!file.exists()) {
        return std::nullopt;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt;
    }
    QByteArray data = file.readAll();
    file.close();

    QRegularExpression pathRegex(R"(\s*\"path\"\s*\"([^\"]+)\")");
    QRegularExpressionMatchIterator matches = pathRegex.globalMatch(QString::fromUtf8(data));
    std::vector<QString> libraryPaths;
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        if (match.hasMatch()) {
            libraryPaths.push_back(match.captured(1));
        }
    }

    QString appManifestFile = QString("appmanifest_%1.acf").arg(gameMetadata.steamAppId);
    for (const QString& lib : libraryPaths) {
        QString fullPath = lib + "/steamapps/" + appManifestFile;
        if (QFile::exists(fullPath)) {
            return lib;
        }
    }
    return std::nullopt;
}

std::optional<QString> getAppManifestVDFPath(const UtilGameSyncServer::GameMetadata gameMetadata) {
    auto maybeGameLib = getGameLibraryFolder(gameMetadata);
    if (maybeGameLib.has_value()) {
        QString appManifestFile = QString("appmanifest_%1.acf").arg(gameMetadata.steamAppId);
        QString fullPath = maybeGameLib.value() + "/steamapps/" + appManifestFile;
        if (QFile::exists(fullPath)) {
            return fullPath;
        }
    }
    return std::nullopt;
}

std::optional<QString> getSteamStoreUserID(const UtilGameSyncServer::GameMetadata gameMetadata) {
    auto maybeAppManifestVDF = getAppManifestVDFPath(gameMetadata);
    if (maybeAppManifestVDF.has_value()) {
        QFile file(maybeAppManifestVDF.value());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {};
        }

        const QByteArray data = file.readAll();
        const QString content = QString::fromUtf8(data);

        const QRegularExpression regex(R"""("LastOwner"\s*"(\d+)")""");
        const QRegularExpressionMatch match = regex.match(content);
        if (!match.hasMatch()) {
            return {};
        }

        return match.captured(1);
    }
    return std::nullopt;
}

#ifdef Q_OS_LINUX
std::optional<QString> getWineBasePath(const UtilGameSyncServer::GameMetadata gameMetadata) {
    auto maybeGameLib = getGameLibraryFolder(gameMetadata);
    if (maybeGameLib.has_value()) {
        QString wineBasePath =
            maybeGameLib.value() +
            QString("/steamapps/compatdata/%1/pfx/drive_c/").arg(gameMetadata.steamAppId);
        if (QFile::exists(wineBasePath)) {
            return wineBasePath;
        }
    }
    return std::nullopt;
}
#endif

QList<QString> getAllRootExpand([[maybe_unused]] const UtilGameSyncServer::GamePath gamePath,
                                const UtilGameSyncServer::GameMetadata) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
#ifdef Q_OS_LINUX
    if (gamePath.operatingSystem == UtilGameSyncServer::linuxOS) {
        QString user = env.value("USER");
        return {"/home/" + user + "/.local/share/Steam/steamapps/common/"};
    } else {
        return {};
    }
#elif defined(Q_OS_WINDOWS)
    QString programFileX86 = env.value("PROGRAMFILES(X86)");
    QString programFile = env.value("PROGRAMFILES");
    return {programFile, programFile + "/Steam/steamapps/common/", programFileX86,
            programFileX86 + "/Steam/steamapps/common/"};
#endif
}

QString getHomeFolder([[maybe_unused]] const UtilGameSyncServer::GameMetadata gameMetadata,
                      [[maybe_unused]] const UtilGameSyncServer::GamePath gamePath) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
#ifdef Q_OS_LINUX
    if (gamePath.operatingSystem == UtilGameSyncServer::linuxOS) {
        return env.value("HOME");
    } else {
        auto winePath = getWineBasePath(gameMetadata);
        if (winePath.has_value()) {
            return winePath.value() + "users/steamuser/";
        }
        return {};
    }
#elif defined(Q_OS_WINDOWS)
    QString homeDrive = env.value("HOMEDRIVE");
    QString homePath = env.value("HOMEPATH");
    return {homeDrive + homePath};
#endif
}

std::optional<QString> getStoreGameId(const UtilGameSyncServer::GameMetadata gameMetadata) {
    if (!gameMetadata.steamAppId.isEmpty()) {
        return gameMetadata.steamAppId;
    }
    return {};
}

std::optional<QString> getStoreUserId(const UtilGameSyncServer::GameMetadata gameMetadata) {
    if (!gameMetadata.steamAppId.isEmpty()) {
        return getSteamStoreUserID(gameMetadata);
    }
    return {};
}

std::optional<QString> getOsUserName([[maybe_unused]] const UtilGameSyncServer::GamePath gamePath) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
#ifdef Q_OS_LINUX
    if (gamePath.operatingSystem == UtilGameSyncServer::linuxOS) {
        return env.value("USER");
    } else {
        return "steamuser";
    }
#elif defined(Q_OS_WINDOWS)
    return env.value("USERNAME");
#endif
}

std::optional<QString>
getWinAppData([[maybe_unused]] const UtilGameSyncServer::GameMetadata gameMetadata) {
#ifdef Q_OS_LINUX
    auto winePath = getWineBasePath(gameMetadata);
    if (winePath.has_value()) {
        return winePath.value() + "users/steamuser/AppData/Roaming/";
    }
    return {};
#elif defined(Q_OS_WINDOWS)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    return env.value("APPDATA");
#endif
}

std::optional<QString>
getWinLocalAppData([[maybe_unused]] const UtilGameSyncServer::GameMetadata gameMetadata) {
#ifdef Q_OS_LINUX
    auto winePath = getWineBasePath(gameMetadata);
    if (winePath.has_value()) {
        return winePath.value() + "users/steamuser/AppData/Local/";
    }
    return {};
#elif defined(Q_OS_WINDOWS)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    return env.value("LOCALAPPDATA");
#endif
}

std::optional<QString>
getWinLocalAppDataLow([[maybe_unused]] const UtilGameSyncServer::GameMetadata gameMetadata,
                      [[maybe_unused]] const UtilGameSyncServer::GamePath gamePath) {
#ifdef Q_OS_LINUX
    auto winePath = getWineBasePath(gameMetadata);
    if (winePath.has_value()) {
        return winePath.value() + "users/steamuser/AppData/LocalLow/";
    }
    return {};
#elif defined(Q_OS_WINDOWS)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    return getHomeFolder(gameMetadata, gamePath) + "/AppData/LocalLow";
#endif
}

std::optional<QString>
getWinDocuments([[maybe_unused]] const UtilGameSyncServer::GameMetadata gameMetadata,
                [[maybe_unused]] const UtilGameSyncServer::GamePath gamePath) {
#ifdef Q_OS_LINUX
    auto winePath = getWineBasePath(gameMetadata);
    if (winePath.has_value()) {
        return winePath.value() + "users/steamuser/Documents/";
    }
    return {};
#elif defined(Q_OS_WINDOWS)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    return getHomeFolder(gameMetadata, gamePath) + "/Documents";
#endif
}

std::optional<QString>
getWinPublic([[maybe_unused]] const UtilGameSyncServer::GameMetadata gameMetadata) {
#ifdef Q_OS_LINUX
    auto winePath = getWineBasePath(gameMetadata);
    if (winePath.has_value()) {
        return winePath.value() + "users/Public/";
    }
    return {};
#elif defined(Q_OS_WINDOWS)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    return env.value("PUBLIC");
#endif
}

std::optional<QString>
getWinProgramData([[maybe_unused]] const UtilGameSyncServer::GameMetadata gameMetadata) {
#ifdef Q_OS_LINUX
    auto winePath = getWineBasePath(gameMetadata);
    if (winePath.has_value()) {
        return winePath.value() + "ProgramData/";
    }
    return {};
#elif defined(Q_OS_WINDOWS)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    return env.value("PROGRAMDATA");
#endif
}

std::optional<QString>
getWinDir([[maybe_unused]] const UtilGameSyncServer::GameMetadata gameMetadata) {
#ifdef Q_OS_LINUX
    auto winePath = getWineBasePath(gameMetadata);
    if (winePath.has_value()) {
        return winePath.value() + "windows/";
    }
    return {};
#elif defined(Q_OS_WINDOWS)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    return env.value("WINDIR");
#endif
}

std::optional<QString> getXdgData() {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
#ifdef Q_OS_LINUX
    return env.value("XDG_DATA_HOME");
#elif defined(Q_OS_WINDOWS)
    return {};
#endif
}

std::optional<QString> getXdgConfig() {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
#ifdef Q_OS_LINUX
    return env.value("XDG_CONFIG_HOME");
#elif defined(Q_OS_WINDOWS)
    return {};
#endif
}

std::optional<QString> expandTagNoRoot(const PathTag tag,
                                       const UtilGameSyncServer::GameMetadata gameMetadata,
                                       const UtilGameSyncServer::GamePath gamePath) {
    switch (tag) {
    case game:
        return gameMetadata.installDir;
    case home:
        return getHomeFolder(gameMetadata, gamePath);
    case storeGameId:
        return getStoreGameId(gameMetadata);
    case storeUserId:
        return getStoreUserId(gameMetadata);
    case osUserName:
        return getOsUserName(gamePath);
    case winAppData:
        return getWinAppData(gameMetadata);
    case winLocalAppData:
        return getWinLocalAppData(gameMetadata);
    case winLocalAppDataLow:
        return getWinLocalAppDataLow(gameMetadata, gamePath);
    case winDocuments:
        return getWinDocuments(gameMetadata, gamePath);
    case winPublic:
        return getWinPublic(gameMetadata);
    case winProgramData:
        return getWinProgramData(gameMetadata);
    case winDir:
        return getWinDir(gameMetadata);
    case xdgData:
        return getXdgData();
    case xdgConfig:
        return getXdgConfig();
    default:
        return {};
    }
}

QString getAutoPath(const UtilGameSyncServer::GameMetadata gameMetadata,
                    const UtilGameSyncServer::GamePath gamePath) {
    QString newPath = gamePath.path;

    newPath.replace("<base>", "<root>/<game>");

    QRegularExpression regex("<[a-zA-Z]*>");
    for (const QRegularExpressionMatch& match : regex.globalMatch(gamePath.path)) {
        auto tag = tagListMap.constFind(match.capturedTexts().constFirst()).value();
        std::optional<QString> maybeExpandedTag = expandTagNoRoot(tag, gameMetadata, gamePath);
        if (maybeExpandedTag)
            newPath.replace(match.capturedTexts().constFirst(), maybeExpandedTag.value());
    }

    if (newPath.contains("<root>")) {
        for (auto roots : getAllRootExpand(gamePath, gameMetadata)) {
            QString pathTry = newPath;
            pathTry.replace("<root>", roots);
            if (utilFileSystem::validatePath(pathTry)) {
                newPath = pathTry;
            }
        }
    }

    return newPath;
}

} // namespace AutoPathFinder

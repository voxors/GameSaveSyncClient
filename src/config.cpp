#include "config.h"

namespace config {
void addId(int newId) {
    QSettings settings = config::getConfig();
    QVariantList ids = settings.value("ids").toList();

    if (!ids.contains(QVariant(newId))) {
        ids.append(QVariant(newId));
        settings.setValue("ids", ids);
    }
}

void removeId(int idToRemove) {
    QSettings settings = getConfig();
    QVariantList ids = settings.value("ids").toList();

    const auto iter = std::ranges::find_if(
        ids, [&](const QVariant& variant) -> bool { return variant.toInt() == idToRemove; });

    if (iter != ids.end()) {
        ids.erase(iter);
        settings.setValue("ids", ids);
    }
}

auto returnAllIds() -> QList<int> {
    QSettings settings = config::getConfig();
    QVariantList ids = settings.value("ids").toList();
    QList<int> idList;
    for (const QVariant& variant : ids) {
        idList.push_back(variant.toInt());
    }
    return idList;
}

auto getPathKey(int pathID) -> QString { return "Path: " + QString::number(pathID); }
auto getPathStringKey(int pathID) -> QString { return getPathKey(pathID) + "/path"; }
auto getPathUUIDKey(int pathID) -> QString { return getPathKey(pathID) + "/last_save"; }

void updatePath(int pathID, QString path) {
    QSettings settings = config::getConfig();
    settings.setValue(getPathStringKey(pathID), path);
}

void removePath(int pathID) {
    QSettings settings = config::getConfig();
    settings.remove(getPathStringKey(pathID));
}

auto getPath(int pathID) -> QString {
    QSettings settings = config::getConfig();
    return settings.value(getPathStringKey(pathID), QString{}).toString();
}

auto getUUIDForPath(int pathID) -> QString {
    QSettings settings = config::getConfig();
    return settings.value(getPathUUIDKey(pathID), QString{}).toString();
}

void updateUUIDForPath(int pathID, QString uuid) {
    QSettings settings = config::getConfig();
    settings.setValue(getPathUUIDKey(pathID), uuid);
}

void removeUUIDForPath(int pathID) {
    QSettings settings = config::getConfig();
    settings.remove(getPathUUIDKey(pathID));
}

void updateRemoteURL(QUrl remoteURL) {
    QSettings settings = config::getConfig();
    settings.setValue("remote/URL", remoteURL);
}

auto getRemoteURL() -> QUrl {
    QSettings settings = config::getConfig();
    return settings.value("remote/URL", QString{}).toUrl();
}

auto validateDbUUID(QString uuid) -> bool {
    QSettings settings = config::getConfig();
    QString configDbUuidPath("db_uuid");
    QString configDbUuid = settings.value(configDbUuidPath, QString{}).toString();
    if (configDbUuid.isEmpty()) {
        settings.setValue(configDbUuidPath, uuid);
        return true;
    } else if (configDbUuid == uuid) {
        return true;
    } else
        return false;
}

void updateAPIToken(QString apiToken) {
    QSettings settings = config::getConfig();
    settings.setValue("remote/APIToken", apiToken);
}

auto getAPIToken() -> QString {
    QSettings settings = config::getConfig();
    return settings.value("remote/APIToken", QString{}).toString();
}

} // namespace config

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

QList<int> returnAllIds() {
    QSettings settings = config::getConfig();
    QVariantList ids = settings.value("ids").toList();
    QList<int> idList;
    for (const QVariant& variant : ids) {
        idList.push_back(variant.toInt());
    }
    return idList;
}

QString getPathKey(int pathID) { return "Path: " + QString::number(pathID); }
QString getPathStringKey(int pathID) { return getPathKey(pathID) + "/path"; }
QString getPathUUIDKey(int pathID) { return getPathKey(pathID) + "/last_save"; }

void updatePath(int pathID, QString path) {
    QSettings settings = config::getConfig();
    settings.setValue(getPathStringKey(pathID), path);
}

void removePath(int pathID) {
    QSettings settings = config::getConfig();
    settings.remove(getPathStringKey(pathID));
}

QString getPath(int pathID) {
    QSettings settings = config::getConfig();
    return settings.value(getPathStringKey(pathID), QString{}).toString();
}

QString getUUIDForPath(int pathID) {
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

QUrl getRemoteURL() {
    QSettings settings = config::getConfig();
    return settings.value("remote/URL", QString{}).toUrl();
}

bool validateDbUUID(QString uuid) {
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

} // namespace config

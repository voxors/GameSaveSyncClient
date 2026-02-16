#pragma once

#include <QSettings>
#include <QUrl>

namespace config {
inline auto getConfig() -> QSettings { return QSettings(); }
auto returnAllIds() -> QList<int>;
auto getPath(int pathID) -> QString;
void addId(int newId);
void removeId(int idToRemove);
void removePath(int pathID);
void updatePath(int pathID, QString path);
auto getUUIDForPath(int pathID) -> QString;
void updateUUIDForPath(int pathID, QString uuid);
void removeUUIDForPath(int pathID);
void updateRemoteURL(QUrl remoteURL);
auto getRemoteURL() -> QUrl;
void updateAPIToken(QString apiToken);
auto getAPIToken() -> QString;
auto validateDbUUID(QString uuid) -> bool;
} // namespace config

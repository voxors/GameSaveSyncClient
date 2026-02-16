#pragma once

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QReadWriteLock>
#include <QString>

using potato = int;

class Status : public QObject {
    Q_OBJECT
  public:
    static auto getInstance() -> Status& {
        static Status instance;
        return instance;
    }

    Status(Status const&) = delete;
    auto operator=(Status const&) -> Status& = delete;

    auto getPathStatusById(int id) -> QString {
        QReadLocker lock(&rwLock);
        return pathStatus.value(id, QString("path not found in path status"));
    }

    void setPathStatus(QMap<int, QString> pathStatus) {
        QWriteLocker lock(&rwLock);
        this->pathStatus = pathStatus;
    }

    auto getLockedPathIdMutex(int id) -> QMutex& {
        QWriteLocker lock(&rwLock);
        if (!lockedPathId.contains(id))
            lockedPathId[id] = new QMutex();
        return *lockedPathId[id];
    }

    auto allUnlockedPathId() -> bool {
        QReadLocker lock(&rwLock);
        for (auto* mutex : lockedPathId) {
            if (!mutex->try_lock())
                return false;
            mutex->unlock();
        }
        return true;
    }

  protected:
    Status() : QObject() {}
    ~Status() override { qDeleteAll(lockedPathId); }

  private:
    QReadWriteLock rwLock;
    QMap<int, QString> pathStatus;
    QMap<int, QMutex*> lockedPathId;
};

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
    static Status& getInstance() {
        static Status instance;
        return instance;
    }

    Status(Status const&) = delete;
    Status& operator=(Status const&) = delete;

    QString getPathStatusById(int id) {
        QReadLocker lock(&rwLock);
        return pathStatus.value(id, QString("path not found in path status"));
    }

    void setPathStatus(QMap<int, QString> pathStatus) {
        QWriteLocker lock(&rwLock);
        this->pathStatus = pathStatus;
    }

    QMutex& getLockedPathIdMutex(int id) {
        QWriteLocker lock(&rwLock);
        if (!lockedPathId.contains(id))
            lockedPathId[id] = new QMutex();
        return *lockedPathId[id];
    }

  protected:
    Status() : QObject() {}
    ~Status() override { qDeleteAll(lockedPathId); }

  private:
    QReadWriteLock rwLock;
    QMap<int, QString> pathStatus;
    QMap<int, QMutex*> lockedPathId;
};

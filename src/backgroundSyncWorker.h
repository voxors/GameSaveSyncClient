#pragma once

#include "error.h"
#include <QMap>
#include <QObject>
#include <QString>
#include <QTimer>
#include <expected>

class BackgroundSyncWorker : public QObject {
    Q_OBJECT

  public:
    BackgroundSyncWorker(QObject* parent = nullptr) : QObject(parent) {}

  public slots:
    void start();
    void update();
    void stop();

  signals:
    void syncFinished();
    void errorOccurred(GameSaveSyncError::Error);

  private:
    QTimer* backgroundTimer;
    void validatePaths();
    std::expected<void, GameSaveSyncError::Error> syncGameSaveToServer();
    std::expected<void, GameSaveSyncError::Error> syncGameSaveFromServer();
};

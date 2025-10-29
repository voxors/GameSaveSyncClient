#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QTimer>

class BackgroundSyncWorker : public QObject {
    Q_OBJECT

  public:
    BackgroundSyncWorker(QObject* parent = nullptr) : QObject(parent) {}

  public slots:
    void start();
    void update();

  signals:
    void syncFinished();
    void errorOccurred(QString);

  private:
    void validatePaths();
    void syncGameSaveToServer();
    void syncGameSaveFromServer();
};

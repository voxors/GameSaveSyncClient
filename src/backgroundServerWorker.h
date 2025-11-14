#pragma once

#include <QLocalServer>
#include <QString>
#include <QThread>

class BackgroundServerWorker : public QObject {
    Q_OBJECT

  public:
    static constexpr QStringView serverName = u"GameSaveSyncClientServerSocket";

    BackgroundServerWorker(QObject* parent = nullptr) : QObject(parent) {}
    ~BackgroundServerWorker() override;

  public slots:
    void start();
    void stop();

  private:
    QLocalServer* server;
};

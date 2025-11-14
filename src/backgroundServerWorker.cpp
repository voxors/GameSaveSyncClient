#include "backgroundServerWorker.h"
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>

BackgroundServerWorker::~BackgroundServerWorker() {
    QLocalServer::removeServer(serverName.toString());
}

void BackgroundServerWorker::start() {
    QLocalServer::removeServer(serverName.toString());

    this->server = new QLocalServer();
    if (!server->listen(serverName.toString())) {
        qWarning() << server->errorString();
        return;
    }
}
void BackgroundServerWorker::stop() { server->close(); };

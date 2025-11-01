#include "backgroundSyncWorker.h"
#include "config.h"
#include "mainWindow.h"
#include "setupWindow.h"
#include "status.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QThread>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("GameSaveSync");
    QCoreApplication::setApplicationName("GameSaveSyncClient");
    QCoreApplication::setApplicationVersion("0.1");

    app.setWindowIcon(QIcon(":/res/icon/GameSaveSyncClientTray.svg"));

    app.setQuitOnLastWindowClosed(false);

    if (!config::getRemoteURL().isValid()) {
        auto* setupWindow = new SetupWindow();
        auto result = static_cast<QDialog::DialogCode>(setupWindow->exec());
        setupWindow->deleteLater();

        if (result != QDialog::Accepted)
            return 0;
    }

    auto workerThread = new QThread;
    auto worker = new BackgroundSyncWorker;
    worker->moveToThread(workerThread);

    QObject::connect(workerThread, &QThread::started, worker, &BackgroundSyncWorker::start);

    auto mainWindow = new MainWindow;

    workerThread->start();

    mainWindow->show();

    int ret = app.exec();

    worker->stop();

    QElapsedTimer timer;
    timer.start();
    constexpr int timeoutMs = 10'000 * 10;
    while (!Status::getInstance().allUnlockedPathId() && timer.elapsed() < timeoutMs) {
        QThread::msleep(10);
    }
    if (timer.elapsed() >= timeoutMs) {
        qWarning() << "Timeout while waiting for all path IDs to unlock.";
    }

    worker->deleteLater();
    workerThread->deleteLater();

    return ret;
}

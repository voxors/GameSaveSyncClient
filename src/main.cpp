#include "backgroundSyncWorker.h"
#include "config.h"
#include "mainWindow.h"
#include "setupWindow.h"
#include <QApplication>
#include <QThread>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("GameSaveSync");
    QCoreApplication::setApplicationName("GameSaveSyncClient");
    QCoreApplication::setApplicationVersion("0.1");

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

    QObject::connect(workerThread, &QThread::started, worker,
                     &BackgroundSyncWorker::start);

    auto mainWindow = new MainWindow;
    QObject::connect(worker, &BackgroundSyncWorker::errorOccurred, mainWindow,
                     &MainWindow::onErrorOccurred);

    workerThread->start();

    mainWindow->show();

    int ret = app.exec();

    worker->deleteLater();
    workerThread->deleteLater();

    return ret;
}

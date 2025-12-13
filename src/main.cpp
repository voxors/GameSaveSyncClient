#include "backgroundServerWorker.h"
#include "backgroundSyncWorker.h"
#include "config.h"
#include "mainWindow.h"
#include "setupWindow.h"
#include "status.h"
#include "trayIcon.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QLocalSocket>
#include <QThread>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("GameSaveSync");
    QCoreApplication::setApplicationName("GameSaveSyncClient");
    QCoreApplication::setApplicationVersion("0.3");

    QCommandLineParser parser;
    parser.setApplicationDescription("GameSaveSyncClient");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption minimizedOption("m", "Start Minimized");
    parser.addOption(minimizedOption);
    parser.process(app);

    app.setWindowIcon(QIcon(":/res/icon/GameSaveSyncClientTray.svg"));

    app.setQuitOnLastWindowClosed(false);

    if (!config::getRemoteURL().isValid()) {
        auto* setupWindow = new SetupWindow();
        auto result = static_cast<QDialog::DialogCode>(setupWindow->exec());
        setupWindow->deleteLater();

        if (result != QDialog::Accepted)
            return 0;
    }

    auto backgroundWorkerThread = new QThread;
    auto backgroundWorker = new BackgroundSyncWorker;
    backgroundWorker->moveToThread(backgroundWorkerThread);
    QObject::connect(backgroundWorkerThread, &QThread::started, backgroundWorker,
                     &BackgroundSyncWorker::start);

    QLocalSocket probeSocket;
    probeSocket.connectToServer(BackgroundServerWorker::serverName.toString());
    if (probeSocket.waitForConnected(100)) {
        qWarning() << "Another instance is already running. Exiting.";
        return 0;
    }
    probeSocket.disconnectFromServer();
    auto serverThread = new QThread;
    auto* serverWorker = new BackgroundServerWorker();
    serverWorker->moveToThread(backgroundWorkerThread);
    QObject::connect(serverThread, &QThread::started, serverWorker, &BackgroundServerWorker::start);

    auto mainWindow = new MainWindow(backgroundWorker);

    auto trayIcon = new TrayIcon();
    trayIcon->addShowMenuItem(mainWindow);
    trayIcon->addShowSetupItem();
    trayIcon->addSeparator();
    trayIcon->addStartStopItem(backgroundWorker);
    trayIcon->addSeparator();
    trayIcon->addQuitMenuItem();

    backgroundWorkerThread->start();
    serverThread->start();

    if (!parser.isSet(minimizedOption)) {
        mainWindow->show();
    }
    int ret = app.exec();

    backgroundWorker->stop();
    serverWorker->stop();

    QElapsedTimer timer;
    timer.start();
    constexpr int timeoutMs = 10'000 * 10;
    while (!Status::getInstance().allUnlockedPathId() && timer.elapsed() < timeoutMs) {
        QThread::msleep(10);
    }
    if (timer.elapsed() >= timeoutMs) {
        qWarning() << "Timeout while waiting for all path IDs to unlock.";
    }

    trayIcon->deleteLater();
    serverWorker->deleteLater();
    serverThread->deleteLater();
    backgroundWorker->deleteLater();
    backgroundWorkerThread->deleteLater();

    return ret;
}

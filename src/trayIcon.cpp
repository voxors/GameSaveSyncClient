#include "trayIcon.h"
#include "setupWindow.h"

#include <QApplication>
#include <QMenu>

TrayIcon::TrayIcon() {
    trayIcon = new QSystemTrayIcon(QIcon::fromTheme("applications-system"));
    trayMenu = new QMenu();

    trayIcon->setContextMenu(trayMenu);
    trayIcon->setIcon(QIcon(":/res/icon/GameSaveSyncClientTray.svg"));
    trayIcon->setToolTip("GameSaveSyncClient");
    trayIcon->show();
}

void TrayIcon::addShowMenuItem(MainWindow* mainWindow) {
    auto showMainWindowAction = new QAction("Show");
    trayMenu->addAction(showMainWindowAction);
    connect(showMainWindowAction, &QAction::triggered, mainWindow, &MainWindow::showWindow);

    connect(trayIcon, &QSystemTrayIcon::activated, this,
            [=](QSystemTrayIcon::ActivationReason reason) -> void {
                if (reason == QSystemTrayIcon::Trigger) {
                    showMainWindowAction->trigger();
                }
            });
};

void TrayIcon::addQuitMenuItem() {
    auto quitAction = new QAction("Quit");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    trayMenu->addAction(quitAction);
}

void TrayIcon::addSeparator() { trayMenu->addSeparator(); }

void TrayIcon::addShowSetupItem() {
    auto showSetupAction = new QAction("Setup");
    trayMenu->addAction(showSetupAction);

    connect(showSetupAction, &QAction::triggered, this, []() -> void {
        auto setupWindowDialog = new SetupWindow();
        setupWindowDialog->setAttribute(Qt::WA_DeleteOnClose);
        setupWindowDialog->show();
    });
}

void TrayIcon::addStartStopItem(BackgroundSyncWorker* backgroundSyncWorker) {
    this->backgroundSyncWorker = backgroundSyncWorker;

    startBackgroundSyncAction = new QAction("Resume Sync");
    stopBackgroundSyncAction = new QAction("Pause Sync");

    connect(startBackgroundSyncAction, &QAction::triggered, this->backgroundSyncWorker,
            &BackgroundSyncWorker::start);
    connect(stopBackgroundSyncAction, &QAction::triggered, this->backgroundSyncWorker,
            &BackgroundSyncWorker::stop);

    trayMenu->addAction(startBackgroundSyncAction);
    trayMenu->addAction(stopBackgroundSyncAction);

    updateSyncActions();

    connect(trayMenu, &QMenu::aboutToShow, this, &TrayIcon::updateSyncActions);
}

void TrayIcon::updateSyncActions() {
    if (!backgroundSyncWorker) {
        startBackgroundSyncAction->setVisible(false);
        stopBackgroundSyncAction->setVisible(false);
        return;
    }

    bool running = backgroundSyncWorker->isRunning();
    startBackgroundSyncAction->setVisible(!running);
    stopBackgroundSyncAction->setVisible(running);
}

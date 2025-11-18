#include "trayIcon.h"

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
    showAction = new QAction("Show");
    trayMenu->addAction(showAction);
    connect(showAction, &QAction::triggered, mainWindow, &MainWindow::showWindow);

    connect(trayIcon, &QSystemTrayIcon::activated, this,
            [&](QSystemTrayIcon::ActivationReason reason) -> void {
                if (reason == QSystemTrayIcon::Trigger) {
                    this->showAction->trigger();
                }
            });
};

void TrayIcon::addQuitMenuItem() {
    quitAction = new QAction("Quit");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    trayMenu->addAction(quitAction);
}

void TrayIcon::addSeparator() { trayMenu->addSeparator(); }

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
    showMainWindowAction = new QAction("Show");
    trayMenu->addAction(showMainWindowAction);
    connect(showMainWindowAction, &QAction::triggered, mainWindow, &MainWindow::showWindow);

    connect(trayIcon, &QSystemTrayIcon::activated, this,
            [&](QSystemTrayIcon::ActivationReason reason) -> void {
                if (reason == QSystemTrayIcon::Trigger) {
                    this->showMainWindowAction->trigger();
                }
            });
};

void TrayIcon::addQuitMenuItem() {
    quitAction = new QAction("Quit");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    trayMenu->addAction(quitAction);
}

void TrayIcon::addSeparator() { trayMenu->addSeparator(); }

void TrayIcon::addShowSetupItem() {
    showSetupAction = new QAction("Setup");
    trayMenu->addAction(showSetupAction);

    connect(showSetupAction, &QAction::triggered, this, []() -> void {
        auto setupWindowDialog = new SetupWindow();
        setupWindowDialog->setAttribute(Qt::WA_DeleteOnClose);
        setupWindowDialog->show();
    });
}

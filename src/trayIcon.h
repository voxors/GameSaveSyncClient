#pragma once

#include "backgroundSyncWorker.h"
#include "mainWindow.h"
#include <QAction>
#include <QSystemTrayIcon>

class TrayIcon : public QObject {
  public:
    TrayIcon();
    void addShowMenuItem(MainWindow* mainWindow);
    void addShowSetupItem();
    void addQuitMenuItem();
    void addStartStopItem(BackgroundSyncWorker* backgroundSyncWorker);
    void addSeparator();

  private:
    QSystemTrayIcon* trayIcon;
    QMenu* trayMenu;
    BackgroundSyncWorker* backgroundSyncWorker;
    QAction* startBackgroundSyncAction;
    QAction* stopBackgroundSyncAction;

    void updateSyncActions();
};

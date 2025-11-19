#pragma once

#include "mainWindow.h"
#include <QAction>
#include <QSystemTrayIcon>

class TrayIcon : public QObject {
  public:
    TrayIcon();
    void addShowMenuItem(MainWindow* mainWindow);
    void addShowSetupItem();
    void addQuitMenuItem();
    void addSeparator();

  private:
    QSystemTrayIcon* trayIcon;
    QMenu* trayMenu;
    QAction* showMainWindowAction;
    QAction* showSetupAction;
    QAction* quitAction;
};

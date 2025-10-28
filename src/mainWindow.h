#pragma once

#include "detailsViewWidget.h"
#include <QAction>
#include <QCloseEvent>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSplitter>
#include <QSystemTrayIcon>
#include <QTreeView>

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

  private:
    DetailsViewWidget* detailsView;
    QAction* aboutQtAction;
    QAction* addGameDialogAction;
    QAction* quitAction;
    QAction* removeGameFromSyncAction;
    QAction* showAction;
    QAction* showSetupWindowAction;
    QListWidget* syncList;
    QMenu* aboutMenu;
    QMenu* fileMenu;
    QMenu* syncMenu;
    QMenu* trayMenu;
    QMenuBar* mainMenuBar;
    QSplitter* mainSplitter;
    QSystemTrayIcon* trayIcon;

    void refreshFromIDFromConfig();

  public slots:
    void onErrorOccurred(QString);
    void showWindow();
  private slots:
    void addGameDialogOpen();
    void removeGameFromSync();
    void showSetupWindowDialog();

  protected:
    void closeEvent(QCloseEvent* event) override; // hide on close
};

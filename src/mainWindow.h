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

  public slots:
    void showWindow();

  signals:
    void connectionIssueSignal(QString message);

  protected:
    void closeEvent(QCloseEvent* event) override;

  private:
    DetailsViewWidget* detailsView;
    QAction* aboutDialogAction;
    QAction* aboutQtAction;
    QAction* addGameDialogAction;
    QAction* quitAction;
    QAction* removeGameFromSyncAction;
    QAction* showSetupWindowAction;
    QListWidget* syncList;
    QMenu* aboutMenu;
    QMenu* fileMenu;
    QMenu* syncMenu;
    QMenuBar* mainMenuBar;
    QSplitter* mainSplitter;

    void refreshFromIDFromConfig();

  private slots:
    void addGameDialogOpen();
    void removeGameFromSync();
    void showSetupWindowDialog();
    void showConnectionError(QString message);
};

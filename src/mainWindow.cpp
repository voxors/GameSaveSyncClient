#include "mainWindow.h"
#include "aboutDialog.h"
#include "addGameDialog.h"
#include "config.h"
#include "detailsViewWidget.h"
#include "setupWindow.h"
#include "utilGameSyncServer.h"
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QIcon>
#include <QKeySequence>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSizePolicy>
#include <QSplitter>
#include <QSystemTrayIcon>
#include <QtLogging>
#include <algorithm>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    mainMenuBar = new QMenuBar(this);

    fileMenu = mainMenuBar->addMenu("&File");
    quitAction = new QAction("Quit", this);
    quitAction->setShortcut(QKeySequence::Quit);
    fileMenu->addAction(quitAction);

    showSetupWindowAction = new QAction("&Setup", this);
    showSetupWindowAction->setStatusTip("Change the configuration of the remote URL");
    connect(showSetupWindowAction, &QAction::triggered, this, &MainWindow::showSetupWindowDialog);
    fileMenu->addAction(showSetupWindowAction);

    syncMenu = mainMenuBar->addMenu("&Sync");
    addGameDialogAction =
        new QAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentNew), "&Add new game", this);
    addGameDialogAction->setShortcut(QKeySequence::New);
    addGameDialogAction->setStatusTip("Add a new game to sync");
    connect(addGameDialogAction, &QAction::triggered, this, &MainWindow::addGameDialogOpen);
    syncMenu->addAction(addGameDialogAction);

    removeGameFromSyncAction =
        new QAction(QIcon::fromTheme(QIcon::ThemeIcon::EditDelete), "&Remove game", this);
    removeGameFromSyncAction->setShortcut(QKeySequence::Delete);
    removeGameFromSyncAction->setStatusTip("Remove a game from the sync list");
    connect(removeGameFromSyncAction, &QAction::triggered, this, &MainWindow::removeGameFromSync);
    syncMenu->addAction(removeGameFromSyncAction);

    aboutMenu = mainMenuBar->addMenu("&About");
    aboutDialogAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::HelpAbout), "&About", this);
    aboutDialogAction->setStatusTip("Show about dialog");
    connect(aboutDialogAction, &QAction::triggered, this, [this]() -> void {
        auto* dialog = new AboutDialog(this);
        dialog->show();
    });
    aboutMenu->addAction(aboutDialogAction);
    aboutQtAction = new QAction("About &Qt", this);
    aboutQtAction->setStatusTip("Show about dialog from Qt");
    connect(aboutQtAction, &QAction::triggered, this,
            [this]() -> void { QMessageBox::aboutQt(this); });
    aboutMenu->addAction(aboutQtAction);

    setMenuBar(mainMenuBar);

    mainSplitter = new QSplitter(this);
    mainSplitter->setSizePolicy({QSizePolicy::Maximum, QSizePolicy::Maximum});
    syncList = new QListWidget(mainSplitter);
    syncList->setSelectionMode(QAbstractItemView::SingleSelection);
    detailsView = new DetailsViewWidget(mainSplitter);

    setCentralWidget(mainSplitter);
    connect(syncList, &QListWidget::itemSelectionChanged, this, [this]() -> void {
        if (auto item = syncList->currentItem()) {
            const int id = item->data(Qt::UserRole).toInt();
            qDebug() << "Selected id:" << id;
            this->detailsView->setGameID(id);
        }
    });

    trayIcon = new QSystemTrayIcon(QIcon::fromTheme("applications-system"), this);
    trayMenu = new QMenu(this);
    showAction = new QAction("Show", this);
    connect(showAction, &QAction::triggered, this, &MainWindow::showWindow);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    connect(trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) -> void {
                if (reason == QSystemTrayIcon::Trigger) {
                    this->showWindow();
                }
            });

    trayMenu->addAction(showAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setIcon(QIcon(":/res/icon/GameSaveSyncClientTray.svg"));
    trayIcon->setToolTip("GameSaveSyncClient");
    trayIcon->show();

    refreshFromIDFromConfig();
}

void MainWindow::addGameDialogOpen() {
    auto dialog = new AddGameDialog(this);
    int id = dialog->exec();
    if (id == 0)
        return;
    config::addId(id);

    refreshFromIDFromConfig();
}

void MainWindow::removeGameFromSync() {
    if (auto item = syncList->currentItem()) {
        const int id = item->data(Qt::UserRole).toInt();
        config::removeId(id);
    }
    refreshFromIDFromConfig();
}

void MainWindow::refreshFromIDFromConfig() {
    syncList->clear();

    QList<UtilGameSyncServer::GameMetadata> gamesMetadata;
    for (auto& id : config::returnAllIds()) {
        auto gameMetadata = UtilGameSyncServer::getInstance().getGameMetadata(id);

        if (gameMetadata.has_value())
            gamesMetadata.append(gameMetadata.value());
    }

    std::ranges::sort(gamesMetadata,
                      [](const UtilGameSyncServer::GameMetadata& value1,
                         const UtilGameSyncServer::GameMetadata& value2) -> int {
                          return QString::compare(value1.defaultName, value2.defaultName,
                                                  Qt::CaseInsensitive) < 0;
                      });

    for (const UtilGameSyncServer::GameMetadata& gameMetadata : gamesMetadata) {
        auto item = new QListWidgetItem(gameMetadata.defaultName, syncList);
        item->setData(Qt::UserRole, gameMetadata.id);
        syncList->addItem(item);
    }

    if (syncList->count() && syncList->selectedItems().empty()) {
        syncList->setCurrentRow(0);
    }
}

void MainWindow::showWindow() {
    this->show();
    this->raise();
    this->activateWindow();
}

void MainWindow::onErrorOccurred(QString msg) { qWarning() << "Background sync error:" << msg; }

void MainWindow::closeEvent(QCloseEvent* event) {
    this->hide();
    event->ignore();
}

void MainWindow::showSetupWindowDialog() {
    auto setupWindowDialog = new SetupWindow(this);
    setupWindowDialog->setAttribute(Qt::WA_DeleteOnClose);
    setupWindowDialog->show();
}

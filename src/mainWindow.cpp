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

MainWindow::MainWindow(BackgroundSyncWorker* backgroundWorker, QWidget* parent)
    : QMainWindow(parent) {
    this->backgroundWorker = backgroundWorker;

    mainMenuBar = new QMenuBar(this);

    fileMenu = mainMenuBar->addMenu("&File");
    quitAction = new QAction("Quit", this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
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

    syncMenu->addSeparator();

    startSyncAction =
        new QAction(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart), "R&esume sync");
    startSyncAction->setStatusTip("Resume the background sync");
    connect(startSyncAction, &QAction::triggered, this, [&]() -> void {
        if (!this->backgroundWorker->isRunning()) {
            this->backgroundWorker->start();
        }
    });
    stopSyncAction =
        new QAction(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackPause), "&Pause sync");
    stopSyncAction->setStatusTip("Pause the background sync");
    connect(stopSyncAction, &QAction::triggered, this, [&]() -> void {
        if (this->backgroundWorker->isRunning()) {
            this->backgroundWorker->stop();
        }
    });

    connect(syncMenu, &QMenu::aboutToShow, this, [&]() -> void {
        if (this->backgroundWorker->isRunning()) {
            startSyncAction->setEnabled(false);
            stopSyncAction->setEnabled(true);
        } else {
            startSyncAction->setEnabled(true);
            stopSyncAction->setEnabled(false);
        }
    });
    syncMenu->addActions({startSyncAction, stopSyncAction});

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
            this->detailsView->setGameID(id);
        }
    });

    connect(this, &MainWindow::connectionIssueSignal, this, &MainWindow::showConnectionError,
            Qt::QueuedConnection);

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

        if (!gameMetadata) {
            emit connectionIssueSignal(gameMetadata.error().message);
            return;
        }

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

    refreshFromIDFromConfig();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    this->hide();
    event->ignore();
}

void MainWindow::showConnectionError(QString message) {
    QMessageBox::critical(this, tr("Game metadata error"), message);
    this->close();
}

void MainWindow::showSetupWindowDialog() {
    auto setupWindowDialog = new SetupWindow(this);
    setupWindowDialog->setAttribute(Qt::WA_DeleteOnClose);
    setupWindowDialog->show();
}

#include "detailsViewWidget.h"
#include "pathItemDelegate.h"
#include "status.h"
#include "utilGameSyncServer.h"
#include <QButtonGroup>
#include <QMutexLocker>
#include <QToolTip>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>

DetailsViewWidget::DetailsViewWidget(QWidget* parent) : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    gameNameLabel = new QLabel(this);
    pathModel = new PathListModel(this);
    pathList = new QListView(this);
    pathList->setModel(pathModel);
    pathList->setEditTriggers(QAbstractItemView::AllEditTriggers);
    pathList->setItemDelegate(new PathItemDelegate());
    auto* pathButtonLayout = new QHBoxLayout();
    forcePullButton = new QPushButton("Force Pull");
    forcePullButton->setToolTip("Delete local save content and replace it with the remote save");
    connect(forcePullButton, &QPushButton::clicked, this, &DetailsViewWidget::forcePull);
    forcePullWatcher = new QFutureWatcher<void>(this);
    connect(forcePullWatcher, &QFutureWatcher<void>::finished, this,
            [this]() -> void { forcePullButton->setEnabled(true); });
    forcePushButton = new QPushButton("Force Push");
    forcePushButton->setToolTip("Delete remote save content and replace it with the local save");
    connect(forcePushButton, &QPushButton::clicked, this, &DetailsViewWidget::forcePush);
    forcePushWatcher = new QFutureWatcher<void>(this);
    connect(forcePushWatcher, &QFutureWatcher<void>::finished, this,
            [this]() -> void { forcePushButton->setEnabled(true); });
    pathButtonLayout->addStretch();
    pathButtonLayout->addWidget(forcePullButton);
    pathButtonLayout->addWidget(forcePushButton);
    executableList = new QListWidget(this);

    mainLayout->addWidget(gameNameLabel);
    auto* separatorLabel = new QFrame();
    separatorLabel->setFrameShape(QFrame::HLine);
    separatorLabel->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separatorLabel);
    mainLayout->addWidget(pathList);
    mainLayout->addLayout(pathButtonLayout);
    auto* separatorPath = new QFrame();
    separatorPath->setFrameShape(QFrame::HLine);
    separatorPath->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separatorPath);
    mainLayout->addWidget(executableList);
}

void DetailsViewWidget::setGameID(int gameID) {
    this->gameID = gameID;
    refresh();
}

void DetailsViewWidget::refresh() {
    if (gameID == 0) {
        return;
    }

    std::optional<UtilGameSyncServer::GameMetadata> maybeMetadata =
        UtilGameSyncServer::getInstance().getGameMetadata(gameID);
    QString gameName = maybeMetadata ? maybeMetadata->defaultName : QString{};
    gameNameLabel->setText(gameName);

    pathModel->loadForGame(gameID);

    QList<UtilGameSyncServer::ExecutableJson> executablesJson =
        UtilGameSyncServer::getInstance().getExecutableByGameId(gameID);
    executableList->clear();
    for (auto executableJson : executablesJson) {
        executableList->addItem(executableJson.executablePath);
    }
}

void DetailsViewWidget::forcePull() {
    if (gameID == 0) {
        return;
    }

    std::optional<QList<UtilGameSyncServer::GamePath>> maybePathList =
        UtilGameSyncServer::getInstance().getPathByGameId(gameID, true);

    if (!maybePathList.has_value())
        return;

    forcePullButton->setEnabled(false);

    QFuture<void> future = QtConcurrent::run([maybePathList]() -> void {
        for (UtilGameSyncServer::GamePath path : maybePathList.value()) {
            QMutexLocker locker(&Status::getInstance().getLockedPathIdMutex(path.id));
            auto result = UtilGameSyncServer::getInstance().fetchLastSaveFromServer(path.id);
        }
    });

    forcePullWatcher->setFuture(future);
}

void DetailsViewWidget::forcePush() {
    if (gameID == 0) {
        return;
    }

    std::optional<QList<UtilGameSyncServer::GamePath>> maybePathList =
        UtilGameSyncServer::getInstance().getPathByGameId(gameID, true);

    if (!maybePathList.has_value())
        return;

    forcePushButton->setEnabled(false);

    QFuture<void> future = QtConcurrent::run([maybePathList]() -> void {
        for (UtilGameSyncServer::GamePath path : maybePathList.value()) {
            QMutexLocker locker(&Status::getInstance().getLockedPathIdMutex(path.id));
            std::expected<void, QString> result =
                UtilGameSyncServer::getInstance().pushLocalSaveToServer(path.id);
            if (!result.has_value())
                qWarning() << "Error while sync Game Save to Server : " + result.error();
        }
    });

    forcePushWatcher->setFuture(future);
}

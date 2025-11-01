#include "addGameDialog.h"
#include "utilGameSyncServer.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>

void addRemoteGameListToSyncList(QList<UtilGameSyncServer::GameMetadata> gamesMetadata,
                                 QListWidget* list) {

    std::ranges::sort(gamesMetadata,
                      [](const UtilGameSyncServer::GameMetadata& value1,
                         const UtilGameSyncServer::GameMetadata& value2) -> int {
                          return QString::compare(value1.defaultName, value2.defaultName,
                                                  Qt::CaseInsensitive) < 0;
                      });

    for (const UtilGameSyncServer::GameMetadata& gameMetadata : gamesMetadata) {
        auto item = new QListWidgetItem(gameMetadata.defaultName, list);
        item->setData(Qt::UserRole, gameMetadata.id);
        list->addItem(item);
    }
}

AddGameDialog::AddGameDialog(QWidget* parent) : QDialog(parent) {
    std::expected<QList<UtilGameSyncServer::GameMetadata>, GameSaveSyncError::Error>
        resultRemoteGameList = UtilGameSyncServer::getInstance().getGameMetadataList();
    QList<UtilGameSyncServer::GameMetadata> remoteGameList = resultRemoteGameList.value();
    setMinimumSize({500, 200});

    syncList = new QListWidget(this);
    syncList->setSelectionMode(QAbstractItemView::SingleSelection);
    addRemoteGameListToSyncList(remoteGameList, syncList);

    setLayout(new QVBoxLayout(this));
    layout()->addWidget(syncList);

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    cancelButton = new QPushButton("Cancel", this);
    connect(cancelButton, &QPushButton::clicked, this, [this]() -> void { this->reject(); });
    buttonLayout->addWidget(cancelButton);
    addButton = new QPushButton("Add", this);
    connect(addButton, &QPushButton::clicked, this, [this]() -> void {
        if (syncList->selectedItems().length() > 0) {
            this->done(syncList->selectedItems().first()->data(Qt::UserRole).toInt());
        }
    });
    buttonLayout->addWidget(addButton);
    layout()->addItem(buttonLayout);
}

AddGameDialog::~AddGameDialog() = default;

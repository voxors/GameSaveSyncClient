#include "addGameDialog.h"
#include "utilGameSyncServer.h"
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

void addRemoteGameListToSyncList(QList<UtilGameSyncServer::GameDefaultName> gamesDefaultName,
                                 QListWidget* list) {

    std::ranges::sort(gamesDefaultName,
                      [](const UtilGameSyncServer::GameDefaultName& value1,
                         const UtilGameSyncServer::GameDefaultName& value2) -> int {
                          return QString::compare(value1.defaultName, value2.defaultName,
                                                  Qt::CaseInsensitive) < 0;
                      });

    for (const UtilGameSyncServer::GameDefaultName& gameDefaultName : gamesDefaultName) {
        auto item = new QListWidgetItem(gameDefaultName.defaultName, list);
        item->setData(Qt::UserRole, gameDefaultName.id);
        list->addItem(item);
    }
}

AddGameDialog::AddGameDialog(QWidget* parent) : QDialog(parent) {
    std::expected<QList<UtilGameSyncServer::GameDefaultName>, GameSaveSyncError::Error>
        resultRemoteGameList = UtilGameSyncServer::getInstance().getGameDefaultNameList();
    QList<UtilGameSyncServer::GameDefaultName> remoteGameList = resultRemoteGameList.value();
    this->filter = {};
    setMinimumSize({500, 200});

    setLayout(new QVBoxLayout(this));

    filterEdit = new QLineEdit(this);
    filterEdit->setPlaceholderText("Game Filter");
    filterEdit->setClearButtonEnabled(true);
    filterTimer = new QTimer(this);
    filterTimer->setSingleShot(true);
    connect(filterTimer, &QTimer::timeout, this, [&]() -> void {
        auto selectedItems = this->syncList->selectedItems();
        std::optional<int> previousId = std::nullopt;
        if (!selectedItems.isEmpty()) {
            previousId = selectedItems.at(0)->data(Qt::UserRole).toInt();
        }
        auto result = UtilGameSyncServer::getInstance().getGameSearchDefaultNameList(this->filter);
        if (result) {
            syncList->clear();
            addRemoteGameListToSyncList(result.value(), syncList);

            if (previousId.has_value()) {
                for (int i = 0; i < syncList->count(); i++) {
                    if (syncList->item(i)->data(Qt::UserRole).toInt() == previousId.value()) {
                        syncList->selectionModel()->setCurrentIndex(
                            syncList->indexFromItem(syncList->item(i)),
                            QItemSelectionModel::SelectionFlag::ClearAndSelect);
                        break;
                    }
                }
            }
        }
    });
    connect(filterEdit, &QLineEdit::textChanged, this, [&](const QString& filter) -> void {
        this->filter = filter;
        filterTimer->start(500);
    });
    layout()->addWidget(filterEdit);

    syncList = new QListWidget(this);
    syncList->setSelectionMode(QAbstractItemView::SingleSelection);
    addRemoteGameListToSyncList(remoteGameList, syncList);

    layout()->addWidget(syncList);

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(5);
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

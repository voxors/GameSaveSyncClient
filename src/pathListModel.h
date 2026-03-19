#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

struct PathItem {
    int id{0};
    QString dbPath;
    QString configPath;
};

class PathListModel : public QAbstractListModel {
    Q_OBJECT
  public:
    enum Role { IdRole = Qt::UserRole + 1, DbPathRole, ConfigPathRole };

    PathListModel(QObject* parent = nullptr);

    [[nodiscard]] auto rowCount(const QModelIndex& parent = QModelIndex()) const -> int override;
    [[nodiscard]] auto data(const QModelIndex& index, int role = Qt::DisplayRole) const
        -> QVariant override;
    auto setData(const QModelIndex& index, const QVariant& value, int role) -> bool override;
    [[nodiscard]] auto flags(const QModelIndex& index) const -> Qt::ItemFlags override;
    [[nodiscard]] auto items() const -> const QList<PathItem>& { return pathItems; }
    void loadForGame(int gameId);

  private:
    QList<PathItem> pathItems;

    [[nodiscard]] auto isPathValid(PathItem item) const -> bool;
};

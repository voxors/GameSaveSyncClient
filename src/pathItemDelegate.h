#pragma once

#include <QStyledItemDelegate>

class PathItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
  public:
    PathItemDelegate(QObject* parent = nullptr);
    auto createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                      const QModelIndex& index) const -> QWidget* override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override;
};

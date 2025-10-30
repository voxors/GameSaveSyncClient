#include "pathItemDelegate.h"
#include "pathEditableRowWidget.h"
#include "pathListModel.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

PathItemDelegate::PathItemDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QWidget* PathItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&,
                                        const QModelIndex& index) const {
    return new PathEditableRowWidget(index.data(PathListModel::IdRole).toInt(), parent);
}

void PathItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    QString value = index.model()->data(index, PathListModel::ConfigPathRole).toString();

    auto lineEdit = editor->findChild<QLineEdit*>();
    if (lineEdit) {
        lineEdit->setText(value);
    }
}

void PathItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                    const QModelIndex& index) const {
    auto lineEdit = editor->findChild<QLineEdit*>();
    if (lineEdit) {
        model->setData(index, lineEdit->text(), PathListModel::ConfigPathRole);
    }
}

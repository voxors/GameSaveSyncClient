#pragma once

#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

class PathEditableRowWidget : public QWidget {
    Q_OBJECT
  public:
    PathEditableRowWidget(int pathId, QWidget* parent = nullptr);

  private:
    int pathId;
    QLineEdit* lineEdit;
    QPushButton* folderDialog;
};

#pragma once

#include "pathListModel.h"
#include <QFutureWatcher>
#include <QLabel>
#include <QListView>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>

class DetailsViewWidget : public QWidget {
    Q_OBJECT
  public:
    DetailsViewWidget(QWidget* parent = nullptr);
    ~DetailsViewWidget() override = default;
    void setGameID(int id);
    void refresh();

  private slots:
    void forcePull();
    void forcePush();

  private:
    int gameID = 0;
    PathListModel* pathModel;
    QLabel* gameNameLabel;
    QListView* pathList;
    QListWidget* executableList;
    QPushButton* forcePullButton;
    QPushButton* forcePushButton;
    QFutureWatcher<void>* forcePullWatcher;
    QFutureWatcher<void>* forcePushWatcher;
};

#include "pathEditableRowWidget.h"
#include <QFileDialog>
#include <QHBoxLayout>

PathEditableRowWidget::PathEditableRowWidget(int pathId, QWidget* parent) : QWidget(parent) {
    this->pathId = pathId;
    this->setAutoFillBackground(true);

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout);

    lineEdit = new QLineEdit();
    lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    folderDialog = new QPushButton("browse...");
    folderDialog->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    connect(folderDialog, &QPushButton::clicked, this, [this]() -> void {
        QWidget* parentWidget = this->parentWidget();
        QString startPath = QDir::homePath();
        QString dir = QFileDialog::getExistingDirectory(parentWidget, "Select directory", startPath,
                                                        QFileDialog::ShowDirsOnly |
                                                            QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            lineEdit->setText(dir);
        }
    });

    this->layout()->addWidget(lineEdit);
    this->layout()->addWidget(folderDialog);
}

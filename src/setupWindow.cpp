#include "setupWindow.h"
#include "config.h"
#include "utilGameSyncServer.h"

#include <QCursor>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QToolTip>
#include <QUrl>
#include <QVBoxLayout>

SetupWindow::SetupWindow(QWidget* parent) : QDialog(parent) {
    urlEdit = new QLineEdit(this);
    portSpin = new QSpinBox(this);
    validateButton = new QPushButton("Validate", this);
    applyButton = new QPushButton("Apply", this);
    cancelButton = new QPushButton("Cancel", this);

    setWindowTitle("Server Configuration");

    QUrl currentUrl = config::getRemoteURL();
    urlEdit->setText(currentUrl.toString(QUrl::RemovePort));
    urlEdit->setPlaceholderText("http://localhost");
    portSpin->setRange(1, 65535);
    portSpin->setValue(currentUrl.port() == -1 ? 3000 : currentUrl.port());

    applyButton->setEnabled(false);

    auto hBoxLayout = new QHBoxLayout();
    hBoxLayout->addWidget(new QLabel("Remote URL:"));
    hBoxLayout->addWidget(urlEdit);
    hBoxLayout->addWidget(new QLabel(":"));
    hBoxLayout->addWidget(portSpin);

    buttonBox = new QDialogButtonBox(Qt::Horizontal, this);
    buttonBox->addButton(validateButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(applyButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(hBoxLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(buttonBox);

    setMinimumWidth(400);
    // Qt seems to calculate the minimum to be 150 after the first resize. Could
    // be fixed later, but for now enforce it from the start
    setFixedHeight(150);

    connect(validateButton, &QPushButton::clicked, this, &SetupWindow::validate);
    connect(applyButton, &QPushButton::clicked, this, &SetupWindow::applyConfig);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(urlEdit, &QLineEdit::textEdited, this, [this](const QString& text) -> void {
        if (!text.isEmpty() && (text.startsWith("http://") || text.startsWith("https://")) &&
            text.endsWith(':')) {
            portSpin->setFocus();
            portSpin->selectAll();
        }
    });
}

void SetupWindow::validate() {
    QUrl url(urlEdit->text());
    url.setPort(portSpin->value());

    if (UtilGameSyncServer::getInstance().testConnection(url)) {
        urlEdit->setToolTip("");
        applyButton->setEnabled(true);
    } else {
        QString message = "Validation failed: unable to contact fetch game endpoint.";
        urlEdit->setToolTip(message);
        QToolTip::showText({QCursor::pos().x() - 30, QCursor::pos().y()}, message, urlEdit);
        applyButton->setEnabled(false);
    }
}

void SetupWindow::applyConfig() {
    QUrl url(urlEdit->text());
    url.setPort(portSpin->value());
    config::updateRemoteURL(url);
    accept();
}

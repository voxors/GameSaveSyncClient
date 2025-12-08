#include "aboutDialog.h"
#include <QCoreApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QtSvgWidgets/QtSvgWidgets>

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("About GameSaveSyncClient");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(14);

    auto* logoLayout = new QHBoxLayout();
    auto* logoSvgWidget = new QSvgWidget(":/res/icon/GameSaveSyncClient.svg");
    logoSvgWidget->renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
    logoSvgWidget->setFixedSize(200, 200);
    logoLayout->addStretch();
    logoLayout->addWidget(logoSvgWidget);
    logoLayout->addStretch();
    layout->addLayout(logoLayout);

    QString applicationName = QCoreApplication::applicationName();
    auto* applicationLabel = new QLabel(applicationName);
    applicationLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(applicationLabel);

    QString version = QCoreApplication::applicationVersion();
    auto* versionLabel = new QLabel("Version " + version);
    versionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(versionLabel);

    auto* licenseLabel = new QLabel();

    QFile licenseFile(":/res/license/LICENSE.txt");
    if (!licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        licenseLabel->setText("License information not available.");
    } else {
        QTextStream stream(&licenseFile);
        stream.setEncoding(QStringConverter::Utf8);
        licenseLabel->setText(stream.readAll());
    }

    licenseLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(licenseLabel);

    auto* thirdPartyButton = new QPushButton(tr("Third‑Party Licenses"), this);
    connect(thirdPartyButton, &QPushButton::clicked, this, [this]() -> void {
        QFile file(":/res/license/Third-Party.txt");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("Error"), tr("Could not open third‑party license file."));
            return;
        }
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        QString text = stream.readAll();

        QDialog thirdPartyDialog(this);
        thirdPartyDialog.setWindowTitle(tr("Third‑Party Licenses"));
        auto* dialogLayout = new QVBoxLayout(&thirdPartyDialog);

        auto* txt = new QTextEdit(&thirdPartyDialog);
        txt->setReadOnly(true);
        txt->setPlainText(text);
        dialogLayout->addWidget(txt);

        auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
        connect(btnBox, &QDialogButtonBox::accepted, &thirdPartyDialog, &QDialog::accept);
        dialogLayout->addWidget(btnBox);

        thirdPartyDialog.resize(600, 500);
        thirdPartyDialog.exec();
    });

    auto* buttonLayout = new QHBoxLayout();
    auto* closeButton = new QPushButton("Ok");
    connect(closeButton, &QPushButton::clicked, this, [&]() -> void { this->close(); });
    buttonLayout->addStretch();
    buttonLayout->addWidget(thirdPartyButton);
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);

    setLayout(layout);

    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->setMinimumSize({650, 750});
    this->setMaximumSize({650, 750});
}

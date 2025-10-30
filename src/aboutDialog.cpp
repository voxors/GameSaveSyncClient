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

    auto* logoSvgWidget = new QSvgWidget(":/res/icon/GameSaveSyncClient.svg");
    logoSvgWidget->renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
    logoSvgWidget->setSizePolicy({QSizePolicy::Maximum, QSizePolicy::Preferred});
    layout->addWidget(logoSvgWidget, Qt::AlignCenter);

    QString applicationName = QCoreApplication::applicationName();
    auto* applicationLabel = new QLabel(applicationName);
    applicationLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(applicationLabel);

    QString version = QCoreApplication::applicationVersion();
    auto* versionLabel = new QLabel("Version " + version);
    versionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(versionLabel);

    auto* licenseLabel = new QLabel(QStringLiteral("License: MIT"));
    licenseLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(licenseLabel);

    auto* buttonLayout = new QHBoxLayout();
    auto* closeButton = new QPushButton("Ok");
    connect(closeButton, &QPushButton::clicked, this, [&]() -> void { this->close(); });
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);

    setLayout(layout);

    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->setMinimumSize({250, 300});
    this->setMaximumSize({250, 300});
}

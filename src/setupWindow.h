#pragma once

#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

class SetupWindow : public QDialog {
    Q_OBJECT

  public:
    SetupWindow(QWidget* parent = nullptr);
    ~SetupWindow() override = default;

  private slots:
    void validate();
    void applyConfig();

  private:
    QLineEdit* urlEdit;
    QSpinBox* portSpin;
    QPushButton* validateButton;
    QPushButton* applyButton;
    QPushButton* cancelButton;
    QDialogButtonBox* buttonBox;
    QFrame* urlFrame;
};

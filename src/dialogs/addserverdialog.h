#pragma once
#include "models/server.h"
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QProgressBar>

class AddServerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddServerDialog(QWidget *parent = nullptr);
    explicit AddServerDialog(const Server &existing, QWidget *parent = nullptr);

    Server server() const;
    QString password() const;       // SSH password (used only for key setup)
    bool setupKeyAuth() const;

private slots:
    void onSetupKeyClicked();
    void onTestConnectionClicked();

private:
    void setupUi();
    void populateFrom(const Server &server);

    QLineEdit   *m_nameEdit;
    QLineEdit   *m_hostEdit;
    QSpinBox    *m_portSpin;
    QLineEdit   *m_usernameEdit;
    QLineEdit   *m_passwordEdit;
    QLineEdit   *m_keyPathEdit;
    QCheckBox   *m_keyAuthCheck;
    QLineEdit   *m_groupEdit;
    QTextEdit   *m_descEdit;
    QPushButton *m_setupKeyBtn;
    QPushButton *m_testBtn;
    QLabel      *m_statusLabel;
    bool        m_setupKeyAuth = false;
};

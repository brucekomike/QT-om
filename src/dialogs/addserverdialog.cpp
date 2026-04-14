#include "addserverdialog.h"
#include "ssh/sshclient.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QCoreApplication>
#include <QUuid>

AddServerDialog::AddServerDialog(QWidget *parent) : QDialog(parent)
{
    setupUi();
}

AddServerDialog::AddServerDialog(const Server &existing, QWidget *parent) : QDialog(parent)
{
    setupUi();
    populateFrom(existing);
}

void AddServerDialog::setupUi()
{
    setWindowTitle(tr("Add / Edit Server"));
    setMinimumWidth(480);

    m_nameEdit     = new QLineEdit(this);
    m_hostEdit     = new QLineEdit(this);
    m_portSpin     = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(22);
    m_usernameEdit = new QLineEdit(this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("Only needed for initial key setup"));
    m_keyPathEdit  = new QLineEdit(this);
    m_keyPathEdit->setPlaceholderText(tr("~/.ssh/id_ed25519 (auto-detected if blank)"));
    m_keyAuthCheck = new QCheckBox(tr("Use SSH key authentication"), this);
    m_groupEdit    = new QLineEdit(this);
    m_descEdit     = new QTextEdit(this);
    m_descEdit->setMaximumHeight(80);

    auto *browseBtn = new QPushButton(tr("Browse…"), this);
    connect(browseBtn, &QPushButton::clicked, this, [this]{
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Select SSH Private Key"), QDir::homePath() + "/.ssh");
        if (!path.isEmpty()) m_keyPathEdit->setText(path);
    });

    auto *keyRow = new QHBoxLayout;
    keyRow->addWidget(m_keyPathEdit);
    keyRow->addWidget(browseBtn);

    m_setupKeyBtn = new QPushButton(tr("Setup Key Auth…"), this);
    m_setupKeyBtn->setToolTip(tr("Automatically configure SSH key authentication using the password above"));
    connect(m_setupKeyBtn, &QPushButton::clicked, this, &AddServerDialog::onSetupKeyClicked);

    m_testBtn = new QPushButton(tr("Test Connection"), this);
    connect(m_testBtn, &QPushButton::clicked, this, &AddServerDialog::onTestConnectionClicked);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);

    auto *form = new QFormLayout;
    form->addRow(tr("Display Name:"), m_nameEdit);
    form->addRow(tr("Host / IP:"),    m_hostEdit);
    form->addRow(tr("Port:"),         m_portSpin);
    form->addRow(tr("Username:"),     m_usernameEdit);
    form->addRow(tr("Password:"),     m_passwordEdit);
    form->addRow(tr("SSH Key:"),      keyRow);
    form->addRow(QString(),           m_keyAuthCheck);
    form->addRow(tr("Group:"),        m_groupEdit);
    form->addRow(tr("Description:"),  m_descEdit);

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(m_setupKeyBtn);
    btnRow->addWidget(m_testBtn);
    btnRow->addStretch();

    auto *bbox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *vbox = new QVBoxLayout(this);
    vbox->addLayout(form);
    vbox->addLayout(btnRow);
    vbox->addWidget(m_statusLabel);
    vbox->addWidget(bbox);
}

void AddServerDialog::populateFrom(const Server &s)
{
    m_nameEdit->setText(s.name);
    m_hostEdit->setText(s.host);
    m_portSpin->setValue(s.port);
    m_usernameEdit->setText(s.username);
    m_keyPathEdit->setText(s.sshKeyPath);
    m_keyAuthCheck->setChecked(s.keyAuthEnabled);
    m_groupEdit->setText(s.group);
    m_descEdit->setPlainText(s.description);
}

Server AddServerDialog::server() const
{
    Server s;
    s.id             = QUuid::createUuid().toString(QUuid::WithoutBraces);
    s.name           = m_nameEdit->text().trimmed();
    s.host           = m_hostEdit->text().trimmed();
    s.port           = m_portSpin->value();
    s.username       = m_usernameEdit->text().trimmed();
    s.sshKeyPath     = m_keyPathEdit->text().trimmed();
    s.keyAuthEnabled = m_keyAuthCheck->isChecked();
    s.group          = m_groupEdit->text().trimmed();
    s.description    = m_descEdit->toPlainText().trimmed();
    s.addedAt        = QDateTime::currentDateTime();
    return s;
}

QString AddServerDialog::password() const
{
    return m_passwordEdit->text();
}

bool AddServerDialog::setupKeyAuth() const
{
    return m_setupKeyAuth;
}

void AddServerDialog::onSetupKeyClicked()
{
    if (m_hostEdit->text().trimmed().isEmpty() ||
        m_usernameEdit->text().trimmed().isEmpty() ||
        m_passwordEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("Input Required"),
            tr("Please fill in Host, Username, and Password before setting up key authentication."));
        return;
    }
    m_setupKeyAuth = true;
    m_statusLabel->setText(tr("Key authentication will be configured when you click OK."));
    m_keyAuthCheck->setChecked(true);
}

void AddServerDialog::onTestConnectionClicked()
{
    const Server s = server();
    if (!s.isValid()) {
        m_statusLabel->setText(tr("Please fill in Host and Username first."));
        return;
    }
    m_statusLabel->setText(tr("Testing connection…"));
    m_testBtn->setEnabled(false);
    QCoreApplication::processEvents();

    SshClient ssh;
    const bool ok = ssh.testConnection(s.host, s.port, s.username, s.sshKeyPath);
    m_testBtn->setEnabled(true);
    if (ok) {
        m_statusLabel->setText(tr("✓ Connection successful!"));
    } else {
        m_statusLabel->setText(tr("✗ Connection failed. Check host, port, username and key."));
    }
}

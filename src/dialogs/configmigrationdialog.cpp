#include "configmigrationdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>

ConfigMigrationDialog::ConfigMigrationDialog(const QList<Server> &servers, QWidget *parent)
    : QDialog(parent), m_servers(servers)
{
    setupUi();
}

void ConfigMigrationDialog::setupUi()
{
    setWindowTitle(tr("Config Migration — Import / Export"));
    setMinimumWidth(460);

    m_passphraseEdit = new QLineEdit(this);
    m_passphraseEdit->setEchoMode(QLineEdit::Password);
    m_passphraseEdit->setPlaceholderText(tr("Optional: leave blank for unencrypted export"));

    auto *exportBtn = new QPushButton(tr("Export Configuration…"), this);
    auto *importBtn = new QPushButton(tr("Import Configuration…"), this);
    m_statusLabel   = new QLabel(this);
    m_statusLabel->setWordWrap(true);

    connect(exportBtn, &QPushButton::clicked, this, &ConfigMigrationDialog::onExportClicked);
    connect(importBtn, &QPushButton::clicked, this, &ConfigMigrationDialog::onImportClicked);

    auto *form = new QFormLayout;
    form->addRow(tr("Passphrase:"), m_passphraseEdit);

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(exportBtn);
    btnRow->addWidget(importBtn);
    btnRow->addStretch();

    auto *bbox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *vbox = new QVBoxLayout(this);
    vbox->addWidget(new QLabel(
        tr("Export your server list to a file for backup or migration to another machine.\n"
           "Use a passphrase to encrypt the file."), this));
    vbox->addLayout(form);
    vbox->addLayout(btnRow);
    vbox->addWidget(m_statusLabel);
    vbox->addWidget(bbox);
}

void ConfigMigrationDialog::onExportClicked()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Configuration"), QDir::homePath() + "/qt-om-config.json",
        tr("JSON Files (*.json);;All Files (*)"));
    if (path.isEmpty()) return;

    ConfigManager mgr(this);
    connect(&mgr, &ConfigManager::statusMessage, m_statusLabel, &QLabel::setText);

    const QString err = mgr.exportConfig(path, m_servers, m_passphraseEdit->text());
    if (!err.isEmpty())
        QMessageBox::critical(this, tr("Export Failed"), err);
    else
        QMessageBox::information(this, tr("Export Complete"),
            tr("Configuration exported to:\n%1").arg(path));
}

void ConfigMigrationDialog::onImportClicked()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Import Configuration"), QDir::homePath(),
        tr("JSON Files (*.json);;All Files (*)"));
    if (path.isEmpty()) return;

    ConfigManager mgr(this);
    connect(&mgr, &ConfigManager::statusMessage, m_statusLabel, &QLabel::setText);

    QString errOut;
    m_importedServers = mgr.importConfig(path, &errOut, m_passphraseEdit->text());

    if (!errOut.isEmpty()) {
        QMessageBox::critical(this, tr("Import Failed"), errOut);
        return;
    }

    QMessageBox::information(this, tr("Import Complete"),
        tr("Imported %1 server(s). Click OK to merge with existing servers.")
            .arg(m_importedServers.size()));
    accept();
}

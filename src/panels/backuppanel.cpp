#include "backuppanel.h"
#include "managers/backupmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>

BackupPanel::BackupPanel(QWidget *parent) : QWidget(parent)
{
    m_serverLabel  = new QLabel(tr("No server selected."), this);
    m_scriptEdit   = new QLineEdit(this);
    m_scriptEdit->setPlaceholderText("~/scripts/backup.sh");
    m_archiveEdit  = new QLineEdit(this);
    m_archiveEdit->setPlaceholderText("/tmp/backup.tar.gz");
    m_localDirEdit = new QLineEdit(this);
    m_localDirEdit->setPlaceholderText(QDir::homePath() + "/qt-om-backups");
    m_cronEdit     = new QLineEdit("0 3 * * *", this);
    m_cronEdit->setPlaceholderText("0 3 * * *  (daily 3am)");
    m_commentEdit  = new QLineEdit("qt-om-backup", this);

    auto *browseBtn = new QPushButton(tr("…"), this);
    browseBtn->setFixedWidth(32);
    connect(browseBtn, &QPushButton::clicked, this, [this]{
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Select Local Backup Directory"), QDir::homePath());
        if (!dir.isEmpty()) m_localDirEdit->setText(dir);
    });
    auto *dirRow = new QHBoxLayout;
    dirRow->addWidget(m_localDirEdit);
    dirRow->addWidget(browseBtn);

    m_runBtn        = new QPushButton(tr("▶ Run Backup Now"), this);
    m_scheduleBtn   = new QPushButton(tr("Schedule (Cron)"), this);
    m_unscheduleBtn = new QPushButton(tr("Remove Schedule"), this);

    m_backupList = new QListWidget(this);
    m_logEdit    = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(120);

    connect(m_runBtn,        &QPushButton::clicked, this, &BackupPanel::onRunBackup);
    connect(m_scheduleBtn,   &QPushButton::clicked, this, &BackupPanel::onSchedule);
    connect(m_unscheduleBtn, &QPushButton::clicked, this, &BackupPanel::onUnschedule);
    connect(m_localDirEdit,  &QLineEdit::editingFinished,
            this, &BackupPanel::onRefreshLocalBackups);

    auto *configBox = new QGroupBox(tr("Backup Configuration"), this);
    auto *form = new QFormLayout(configBox);
    form->addRow(tr("Remote Script:"),  m_scriptEdit);
    form->addRow(tr("Remote Archive:"), m_archiveEdit);
    form->addRow(tr("Local Save Dir:"), dirRow);
    form->addRow(tr("Cron Schedule:"),  m_cronEdit);
    form->addRow(tr("Schedule ID:"),    m_commentEdit);

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(m_runBtn);
    btnRow->addWidget(m_scheduleBtn);
    btnRow->addWidget(m_unscheduleBtn);
    btnRow->addStretch();

    auto *vbox = new QVBoxLayout(this);
    vbox->addWidget(m_serverLabel);
    vbox->addWidget(configBox);
    vbox->addLayout(btnRow);
    vbox->addWidget(new QLabel(tr("Local Backups:"), this));
    vbox->addWidget(m_backupList, 1);
    vbox->addWidget(m_logEdit);
}

void BackupPanel::setCurrentServer(const Server &server)
{
    m_server = server;
    m_serverLabel->setText(tr("Server: <b>%1</b> (%2)")
        .arg(server.name.isEmpty() ? server.host : server.name)
        .arg(server.host));
    onRefreshLocalBackups();
}

void BackupPanel::onRunBackup()
{
    if (!m_server.isValid()) {
        QMessageBox::information(this, tr("No Server"), tr("Please select a server first."));
        return;
    }
    BackupConfig cfg;
    cfg.remoteScriptPath  = m_scriptEdit->text().trimmed();
    cfg.remoteArchivePath = m_archiveEdit->text().trimmed();
    cfg.localDestDir      = m_localDirEdit->text().trimmed();
    cfg.scheduleComment   = m_commentEdit->text().trimmed();

    if (cfg.remoteScriptPath.isEmpty() || cfg.remoteArchivePath.isEmpty()
        || cfg.localDestDir.isEmpty()) {
        QMessageBox::warning(this, tr("Input Required"),
            tr("Please fill in Remote Script, Remote Archive, and Local Save Dir."));
        return;
    }

    m_runBtn->setEnabled(false);
    BackupManager mgr(this);
    connect(&mgr, &BackupManager::progressLine, m_logEdit, &QTextEdit::append);
    const QString err = mgr.runBackup(m_server, cfg);
    m_runBtn->setEnabled(true);

    if (!err.isEmpty())
        QMessageBox::warning(this, tr("Backup Failed"), err);
    else
        QMessageBox::information(this, tr("Done"), tr("Backup completed successfully."));

    onRefreshLocalBackups();
}

void BackupPanel::onSchedule()
{
    if (!m_server.isValid()) {
        QMessageBox::information(this, tr("No Server"), tr("Please select a server first."));
        return;
    }
    BackupConfig cfg;
    cfg.remoteScriptPath = m_scriptEdit->text().trimmed();
    cfg.cronSchedule     = m_cronEdit->text().trimmed();
    cfg.scheduleComment  = m_commentEdit->text().trimmed();

    BackupManager mgr(this);
    connect(&mgr, &BackupManager::progressLine, m_logEdit, &QTextEdit::append);
    const QString err = mgr.scheduleBackup(m_server, cfg);
    if (!err.isEmpty())
        QMessageBox::warning(this, tr("Schedule Failed"), err);
    else
        QMessageBox::information(this, tr("Scheduled"),
            tr("Backup scheduled: %1").arg(cfg.cronSchedule));
}

void BackupPanel::onUnschedule()
{
    if (!m_server.isValid()) return;
    BackupConfig cfg;
    cfg.scheduleComment = m_commentEdit->text().trimmed();

    BackupManager mgr(this);
    connect(&mgr, &BackupManager::progressLine, m_logEdit, &QTextEdit::append);
    const QString err = mgr.unscheduleBackup(m_server, cfg);
    if (!err.isEmpty())
        QMessageBox::warning(this, tr("Error"), err);
}

void BackupPanel::onRefreshLocalBackups()
{
    m_backupList->clear();
    BackupConfig cfg;
    cfg.localDestDir = m_localDirEdit->text().trimmed();
    BackupManager mgr(this);
    for (const QString &f : mgr.listLocalBackups(cfg))
        m_backupList->addItem(f);
}

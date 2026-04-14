#pragma once
#include "models/server.h"
#include "ssh/sshclient.h"
#include <QObject>
#include <QString>
#include <QDateTime>

struct BackupConfig {
    QString remoteScriptPath;  // Script to run on server to create backup archive
    QString remoteArchivePath; // Path the script writes the archive to
    QString localDestDir;      // Local directory to save downloaded archives
    QString scheduleComment;   // Identifier for the cron job
    QString cronSchedule;      // Cron expression, e.g. "0 3 * * *"
};

class BackupManager : public QObject
{
    Q_OBJECT
public:
    explicit BackupManager(QObject *parent = nullptr);

    // Run backup now: execute remote script then download the archive
    QString runBackup(const Server &server, const BackupConfig &config);

    // Schedule automatic backup via cron
    QString scheduleBackup(const Server &server, const BackupConfig &config);

    // Remove scheduled backup cron job
    QString unscheduleBackup(const Server &server, const BackupConfig &config);

    // List local backup archives for a config
    QStringList listLocalBackups(const BackupConfig &config);

signals:
    void progressLine(const QString &line);
    void downloadProgress(qint64 received, qint64 total);

private:
    SshClient *m_ssh;
};

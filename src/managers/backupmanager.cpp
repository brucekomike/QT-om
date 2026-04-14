#include "backupmanager.h"
#include "cronmanager.h"
#include <QDir>
#include <QFileInfo>
#include <QDateTime>

BackupManager::BackupManager(QObject *parent)
    : QObject(parent)
    , m_ssh(new SshClient(this))
{
    connect(m_ssh, &SshClient::outputLine, this, &BackupManager::progressLine);
}

QString BackupManager::runBackup(const Server &server, const BackupConfig &config)
{
    if (!server.isValid())
        return tr("Invalid server configuration.");
    if (config.remoteScriptPath.isEmpty())
        return tr("Remote backup script path is required.");
    if (config.remoteArchivePath.isEmpty())
        return tr("Remote archive output path is required.");
    if (config.localDestDir.isEmpty())
        return tr("Local destination directory is required.");

    // Execute backup script on server
    emit progressLine(tr("Running backup script on %1…").arg(server.host));
    const QString execCmd = QString("bash '%1'").arg(config.remoteScriptPath);
    SshClient::Result r = m_ssh->runCommand(
        server.host, server.port, server.username, server.sshKeyPath, execCmd);
    if (!r.success)
        return tr("Backup script failed: %1\n%2")
                   .arg(r.output.trimmed(), r.errorOutput.trimmed());

    // Build local file path with timestamp
    QDir().mkpath(config.localDestDir);
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    const QFileInfo remoteInfo(config.remoteArchivePath);
    const QString localFile  = config.localDestDir + "/" +
                               remoteInfo.baseName() + "_" + timestamp + "." +
                               remoteInfo.completeSuffix();

    emit progressLine(tr("Downloading archive from server…"));
    r = m_ssh->downloadFile(server.host, server.port, server.username,
                            server.sshKeyPath, config.remoteArchivePath, localFile);
    if (!r.success)
        return tr("Download failed: %1").arg(r.errorOutput.trimmed());

    emit progressLine(tr("Backup complete: %1").arg(localFile));
    return {};
}

QString BackupManager::scheduleBackup(const Server &server, const BackupConfig &config)
{
    if (config.cronSchedule.isEmpty())
        return tr("Cron schedule expression is required.");

    CronJob job;
    job.schedule = config.cronSchedule;
    job.command  = QString("bash '%1'").arg(config.remoteScriptPath);
    job.comment  = config.scheduleComment.isEmpty()
                       ? "qt-om-backup"
                       : config.scheduleComment;

    CronManager cronMgr(this);
    connect(&cronMgr, &CronManager::progressLine, this, &BackupManager::progressLine);
    return cronMgr.addOrUpdateCronJob(server, job);
}

QString BackupManager::unscheduleBackup(const Server &server, const BackupConfig &config)
{
    const QString comment = config.scheduleComment.isEmpty()
                                ? "qt-om-backup"
                                : config.scheduleComment;
    CronManager cronMgr(this);
    connect(&cronMgr, &CronManager::progressLine, this, &BackupManager::progressLine);
    return cronMgr.removeCronJob(server, comment);
}

QStringList BackupManager::listLocalBackups(const BackupConfig &config)
{
    if (config.localDestDir.isEmpty()) return {};
    const QDir dir(config.localDestDir);
    if (!dir.exists()) return {};
    QStringList result;
    for (const QFileInfo &fi : dir.entryInfoList(QDir::Files, QDir::Time))
        result << fi.absoluteFilePath();
    return result;
}

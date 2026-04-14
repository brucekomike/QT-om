#pragma once
#include "models/server.h"
#include "ssh/sshclient.h"
#include <QObject>
#include <QString>
#include <QList>

struct CronJob {
    QString schedule;   // e.g. "0 2 * * *"
    QString command;    // command to run
    QString comment;    // optional description

    QString toLine() const;
    static CronJob fromLine(const QString &line);
};

class CronManager : public QObject
{
    Q_OBJECT
public:
    explicit CronManager(QObject *parent = nullptr);

    // Fetch all cron jobs for a user on a remote server
    QList<CronJob> listCronJobs(const Server &server, QString *errorOut = nullptr);

    // Add or replace a cron job (matched by comment as ID)
    QString addOrUpdateCronJob(const Server &server, const CronJob &job);

    // Remove a cron job by its comment identifier
    QString removeCronJob(const Server &server, const QString &comment);

    // Replace the entire crontab on the server
    QString setCrontab(const Server &server, const QList<CronJob> &jobs);

signals:
    void progressLine(const QString &line);

private:
    SshClient *m_ssh;
};

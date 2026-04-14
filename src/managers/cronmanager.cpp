#include "cronmanager.h"
#include <QRegularExpression>

// ── CronJob helpers ──────────────────────────────────────────────────────────

QString CronJob::toLine() const
{
    QString line = schedule + " " + command;
    if (!comment.isEmpty())
        line += " # " + comment;
    return line;
}

CronJob CronJob::fromLine(const QString &rawLine)
{
    const QString line = rawLine.trimmed();
    if (line.startsWith('#') || line.isEmpty()) return {};

    CronJob job;
    // Split schedule (5 fields) from command
    static const QRegularExpression re(
        R"(^(\S+\s+\S+\s+\S+\s+\S+\s+\S+)\s+(.+)$)");
    const auto m = re.match(line);
    if (!m.hasMatch()) return {};

    job.schedule = m.captured(1);
    QString rest = m.captured(2);

    // Extract inline comment as the job comment/id
    const int hashIdx = rest.indexOf(" # ");
    if (hashIdx != -1) {
        job.comment = rest.mid(hashIdx + 3).trimmed();
        rest        = rest.left(hashIdx).trimmed();
    }
    job.command = rest;
    return job;
}

// ── CronManager ──────────────────────────────────────────────────────────────

CronManager::CronManager(QObject *parent)
    : QObject(parent)
    , m_ssh(new SshClient(this))
{
    connect(m_ssh, &SshClient::outputLine, this, &CronManager::progressLine);
}

QList<CronJob> CronManager::listCronJobs(const Server &server, QString *errorOut)
{
    const SshClient::Result r = m_ssh->runCommand(
        server.host, server.port, server.username, server.sshKeyPath,
        "crontab -l 2>/dev/null || true");

    if (!r.success) {
        if (errorOut) *errorOut = r.errorOutput.trimmed();
        return {};
    }

    QList<CronJob> jobs;
    for (const QString &line : r.output.split('\n')) {
        const CronJob job = CronJob::fromLine(line);
        if (!job.schedule.isEmpty())
            jobs.append(job);
    }
    return jobs;
}

QString CronManager::addOrUpdateCronJob(const Server &server, const CronJob &job)
{
    QString errOut;
    QList<CronJob> jobs = listCronJobs(server, &errOut);

    // Replace existing job with same comment, or append
    bool replaced = false;
    if (!job.comment.isEmpty()) {
        for (CronJob &existing : jobs) {
            if (existing.comment == job.comment) {
                existing  = job;
                replaced = true;
                break;
            }
        }
    }
    if (!replaced) jobs.append(job);

    return setCrontab(server, jobs);
}

QString CronManager::removeCronJob(const Server &server, const QString &comment)
{
    QString errOut;
    QList<CronJob> jobs = listCronJobs(server, &errOut);
    jobs.removeIf([&](const CronJob &j){ return j.comment == comment; });
    return setCrontab(server, jobs);
}

QString CronManager::setCrontab(const Server &server, const QList<CronJob> &jobs)
{
    QStringList lines;
    for (const CronJob &j : jobs)
        lines << j.toLine();
    const QString crontab = lines.join('\n') + '\n';

    // Pipe new crontab through stdin
    const QString cmd = QString("echo %1 | crontab -")
                            .arg(QString(crontab).replace('\n', "\\n"));

    // Use here-doc approach which is more reliable
    const QString hereDoc = QString(
        "crontab - << 'QTOM_EOF'\n%1QTOM_EOF").arg(crontab);

    const SshClient::Result r = m_ssh->runCommand(
        server.host, server.port, server.username, server.sshKeyPath, hereDoc);

    if (!r.success)
        return tr("Failed to update crontab: %1").arg(r.errorOutput.trimmed());
    emit progressLine(tr("Crontab updated on %1.").arg(server.host));
    return {};
}

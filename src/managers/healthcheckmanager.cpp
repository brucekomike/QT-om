#include "healthcheckmanager.h"
#include <QRegularExpression>

HealthCheckManager::HealthCheckManager(QObject *parent)
    : QObject(parent)
    , m_ssh(new SshClient(this))
{
    connect(m_ssh, &SshClient::outputLine, this, &HealthCheckManager::progressLine);
}

// Remote script that outputs structured metrics
static const char *HEALTH_SCRIPT = R"(
echo "UPTIME=$(uptime -p 2>/dev/null || uptime)"
echo "CPU=$(top -bn1 | grep 'Cpu(s)' | awk '{print $2}' | tr -d '%us,' )"
MEM_LINE=$(free -m | grep '^Mem:')
echo "MEM_USED=$(echo $MEM_LINE | awk '{print $3}')"
echo "MEM_TOTAL=$(echo $MEM_LINE | awk '{print $2}')"
DISK_LINE=$(df -BG / | tail -1)
echo "DISK_USED=$(echo $DISK_LINE | awk '{print $3}' | tr -d 'G')"
echo "DISK_TOTAL=$(echo $DISK_LINE | awk '{print $2}' | tr -d 'G')"
)";

HealthStatus HealthCheckManager::checkServer(const Server &server)
{
    HealthStatus status;
    status.serverId   = server.id;
    status.serverName = server.name.isEmpty() ? server.host : server.name;
    status.checkedAt  = QDateTime::currentDateTime();

    if (!server.isValid()) {
        status.errorMessage = tr("Invalid server configuration.");
        return status;
    }

    emit progressLine(tr("Checking %1…").arg(server.host));
    const SshClient::Result r = m_ssh->runCommand(
        server.host, server.port, server.username, server.sshKeyPath, HEALTH_SCRIPT);

    if (!r.success) {
        status.reachable    = false;
        status.errorMessage = r.errorOutput.trimmed();
        return status;
    }

    status.reachable = true;
    return parseMetrics(server.id, status.serverName, r.output);
}

void HealthCheckManager::checkAll(const QList<Server> &servers)
{
    for (const Server &s : servers) {
        const HealthStatus hs = checkServer(s);
        emit healthChecked(hs);
    }
}

HealthStatus HealthCheckManager::parseMetrics(const QString &serverId,
                                               const QString &serverName,
                                               const QString &output)
{
    HealthStatus status;
    status.serverId   = serverId;
    status.serverName = serverName;
    status.reachable  = true;
    status.checkedAt  = QDateTime::currentDateTime();

    for (const QString &line : output.split('\n')) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith("UPTIME="))
            status.uptimeStr = trimmed.mid(7);
        else if (trimmed.startsWith("CPU="))
            status.cpuPercent = trimmed.mid(4).toDouble();
        else if (trimmed.startsWith("MEM_USED="))
            status.memUsedMb = trimmed.mid(9).toDouble();
        else if (trimmed.startsWith("MEM_TOTAL="))
            status.memTotalMb = trimmed.mid(10).toDouble();
        else if (trimmed.startsWith("DISK_USED="))
            status.diskUsedGb = trimmed.mid(10).toDouble();
        else if (trimmed.startsWith("DISK_TOTAL="))
            status.diskTotalGb = trimmed.mid(11).toDouble();
    }
    return status;
}

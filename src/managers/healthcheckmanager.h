#pragma once
#include "models/server.h"
#include "ssh/sshclient.h"
#include <QObject>
#include <QString>
#include <QDateTime>

struct HealthStatus {
    QString serverId;
    QString serverName;
    bool    reachable    = false;
    double  cpuPercent   = 0.0;
    double  memUsedMb    = 0.0;
    double  memTotalMb   = 0.0;
    double  diskUsedGb   = 0.0;
    double  diskTotalGb  = 0.0;
    QString uptimeStr;
    QDateTime checkedAt;
    QString errorMessage;
};

class HealthCheckManager : public QObject
{
    Q_OBJECT
public:
    explicit HealthCheckManager(QObject *parent = nullptr);

    // Check a single server and return its status
    HealthStatus checkServer(const Server &server);

    // Check multiple servers (emits healthChecked for each)
    void checkAll(const QList<Server> &servers);

signals:
    void healthChecked(const HealthStatus &status);
    void progressLine(const QString &line);

private:
    HealthStatus parseMetrics(const QString &serverId, const QString &serverName,
                              const QString &output);
    SshClient *m_ssh;
};

#pragma once
#include "models/server.h"
#include "ssh/sshclient.h"
#include <QObject>
#include <QString>

// ServiceManager deploys applications on remote servers using a Git repository
// convention: the repo must contain a deploy/ directory with deploy.sh
class ServiceManager : public QObject
{
    Q_OBJECT
public:
    struct DeployConfig {
        QString repoUrl;      // Git repo URL
        QString repoBranch = "main";
        QString remotePath;   // Where to clone/pull on server, e.g. ~/apps/myapp
        QString deployScript = "deploy/deploy.sh";
        QStringList envVars;  // KEY=VALUE pairs passed as environment
    };

    explicit ServiceManager(QObject *parent = nullptr);

    // Deploy (or update) a service on a server
    // Returns empty string on success, error message otherwise
    QString deployService(const Server &server, const DeployConfig &config);

    // List services installed on a remote server (reads ~/apps/)
    QStringList listInstalledServices(const Server &server);

    // Remove a service from server
    QString removeService(const Server &server, const QString &remotePath);

signals:
    void progressLine(const QString &line);

private:
    SshClient *m_ssh;
};

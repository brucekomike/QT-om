#include "servicemanager.h"

ServiceManager::ServiceManager(QObject *parent)
    : QObject(parent)
    , m_ssh(new SshClient(this))
{
    connect(m_ssh, &SshClient::outputLine, this, &ServiceManager::progressLine);
}

QString ServiceManager::deployService(const Server &server, const DeployConfig &config)
{
    if (!server.isValid())
        return tr("Invalid server configuration.");
    if (config.repoUrl.isEmpty())
        return tr("Git repository URL is required.");
    if (config.remotePath.isEmpty())
        return tr("Remote deployment path is required.");

    // Build environment prefix
    QString envPrefix;
    for (const QString &kv : config.envVars)
        envPrefix += "export " + kv + " && ";

    // Clone or pull
    const QString cloneOrPull = QString(
        "if [ -d '%1/.git' ]; then "
        "  git -C '%1' fetch origin && git -C '%1' reset --hard origin/%2; "
        "else "
        "  git clone --branch %2 %3 %1; "
        "fi"
    ).arg(config.remotePath, config.repoBranch, config.repoUrl);

    emit progressLine(tr("Cloning/updating repository on %1…").arg(server.host));
    SshClient::Result r = m_ssh->runCommand(server.host, server.port, server.username,
                                            server.sshKeyPath, cloneOrPull);
    if (!r.success)
        return tr("Git operation failed: %1").arg(r.errorOutput.trimmed());

    // Run deploy script
    const QString deployCmd = QString(
        "cd '%1' && %2 chmod +x %3 && bash %3"
    ).arg(config.remotePath, envPrefix, config.deployScript);

    emit progressLine(tr("Running deploy script…"));
    r = m_ssh->runCommand(server.host, server.port, server.username,
                          server.sshKeyPath, deployCmd);
    if (!r.success)
        return tr("Deploy script failed: %1\n%2")
                   .arg(r.output.trimmed(), r.errorOutput.trimmed());

    emit progressLine(tr("Deployment successful."));
    return {};
}

QStringList ServiceManager::listInstalledServices(const Server &server)
{
    const SshClient::Result r = m_ssh->runCommand(
        server.host, server.port, server.username, server.sshKeyPath,
        "ls ~/apps/ 2>/dev/null");
    if (!r.success) return {};
    QStringList result;
    for (const QString &line : r.output.split('\n')) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) result << trimmed;
    }
    return result;
}

QString ServiceManager::removeService(const Server &server, const QString &remotePath)
{
    const QString cmd = QString("rm -rf '%1'").arg(remotePath);
    const SshClient::Result r = m_ssh->runCommand(
        server.host, server.port, server.username, server.sshKeyPath, cmd);
    if (!r.success)
        return tr("Failed to remove service: %1").arg(r.errorOutput.trimmed());
    return {};
}

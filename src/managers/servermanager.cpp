#include "servermanager.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>

static QString configDir()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir;
}

ServerManager::ServerManager(QObject *parent)
    : QObject(parent)
    , m_model(new ServerListModel(this))
    , m_ssh(new SshClient(this))
{
    connect(m_ssh, &SshClient::outputLine, this, &ServerManager::statusMessage);
    load();
}

QString ServerManager::dataFilePath()
{
    return configDir() + "/servers.json";
}

void ServerManager::load()
{
    QFile f(dataFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    const QJsonArray arr = doc.array();
    for (const QJsonValue &v : arr)
        m_servers.append(Server::fromJson(v.toObject()));
    m_model->setServers(m_servers);
}

void ServerManager::save()
{
    QJsonArray arr;
    for (const Server &s : m_servers)
        arr.append(s.toJson());
    QFile f(dataFilePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(arr).toJson());
}

const QList<Server> &ServerManager::servers() const { return m_servers; }

Server ServerManager::serverById(const QString &id) const
{
    for (const Server &s : m_servers)
        if (s.id == id) return s;
    return {};
}

void ServerManager::addServer(Server server)
{
    if (server.id.isEmpty())
        server.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!server.addedAt.isValid())
        server.addedAt = QDateTime::currentDateTime();
    m_servers.append(server);
    m_model->addServer(server);
    save();
    emit serverAdded(server);
}

void ServerManager::updateServer(const Server &server)
{
    for (Server &s : m_servers) {
        if (s.id == server.id) {
            s = server;
            m_model->updateServer(server);
            save();
            emit serverUpdated(server);
            return;
        }
    }
}

void ServerManager::removeServer(const QString &id)
{
    m_servers.removeIf([&](const Server &s){ return s.id == id; });
    m_model->removeServer(id);
    save();
    emit serverRemoved(id);
}

QString ServerManager::setupSshKeyAuth(const QString &serverId,
                                       const QString &password,
                                       bool generateNewKey)
{
    Server server = serverById(serverId);
    if (!server.isValid())
        return tr("Server not found.");

    // Determine or generate key
    QString privateKey = server.sshKeyPath;
    if (privateKey.isEmpty() && !generateNewKey)
        privateKey = SshClient::defaultPrivateKeyPath();

    if (privateKey.isEmpty() || generateNewKey) {
        emit statusMessage(tr("Generating new SSH key pair…"));
        privateKey = SshClient::generateKeyPair(
            QString("qt-om@%1").arg(server.host));
        if (privateKey.isEmpty())
            return tr("Failed to generate SSH key pair. Ensure ssh-keygen is installed.");
    }

    const QString publicKey = privateKey + ".pub";
    if (!QFileInfo::exists(publicKey))
        return tr("Public key not found: %1").arg(publicKey);

    emit statusMessage(tr("Copying public key to %1…").arg(server.host));
    const SshClient::Result r = m_ssh->copyPublicKey(
        server.host, server.port, server.username, password, publicKey);

    if (!r.success)
        return tr("Failed to copy public key: %1").arg(r.errorOutput.trimmed());

    // Verify key works
    emit statusMessage(tr("Verifying key-based authentication…"));
    if (!m_ssh->testConnection(server.host, server.port, server.username, privateKey))
        return tr("Key was copied, but test connection failed. Check server configuration.");

    // Persist
    server.sshKeyPath     = privateKey;
    server.keyAuthEnabled = true;
    updateServer(server);
    emit statusMessage(tr("SSH key authentication configured successfully."));
    return {};
}

bool ServerManager::testConnection(const Server &server)
{
    if (!server.isValid()) return false;
    return m_ssh->testConnection(server.host, server.port,
                                 server.username, server.sshKeyPath);
}

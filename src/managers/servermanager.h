#pragma once
#include "models/server.h"
#include "models/serverlistmodel.h"
#include "ssh/sshclient.h"
#include <QObject>
#include <QString>
#include <QList>

class ServerManager : public QObject
{
    Q_OBJECT
public:
    explicit ServerManager(QObject *parent = nullptr);

    // CRUD
    const QList<Server> &servers() const;
    Server serverById(const QString &id) const;
    void addServer(Server server);
    void updateServer(const Server &server);
    void removeServer(const QString &id);

    // SSH key authentication setup:
    // 1. Connect with password, generate key if needed, push public key to server
    // Returns empty string on success, or error message
    QString setupSshKeyAuth(const QString &serverId,
                            const QString &password,
                            bool generateNewKey = false);

    // Test current connection of a server (key or password)
    bool testConnection(const Server &server);

    ServerListModel *model() { return m_model; }

signals:
    void serverAdded(const Server &server);
    void serverUpdated(const Server &server);
    void serverRemoved(const QString &id);
    void statusMessage(const QString &msg);

private:
    void load();
    void save();
    static QString dataFilePath();

    QList<Server>   m_servers;
    ServerListModel *m_model;
    SshClient       *m_ssh;
};

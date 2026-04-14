#pragma once
#include "server.h"
#include <QAbstractListModel>
#include <QList>

class ServerListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        HostRole,
        PortRole,
        UsernameRole,
        KeyAuthRole,
        GroupRole,
        DescriptionRole,
    };

    explicit ServerListModel(QObject *parent = nullptr);

    int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setServers(const QList<Server> &servers);
    void addServer(const Server &server);
    void updateServer(const Server &server);
    void removeServer(const QString &id);
    Server serverAt(int row) const;
    const QList<Server> &servers() const { return m_servers; }

private:
    QList<Server> m_servers;
};

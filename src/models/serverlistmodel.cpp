#include "serverlistmodel.h"

ServerListModel::ServerListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int ServerListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_servers.size();
}

QVariant ServerListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_servers.size())
        return {};
    const Server &s = m_servers.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:        return s.name.isEmpty() ? s.host : s.name;
    case IdRole:          return s.id;
    case HostRole:        return s.host;
    case PortRole:        return s.port;
    case UsernameRole:    return s.username;
    case KeyAuthRole:     return s.keyAuthEnabled;
    case GroupRole:       return s.group;
    case DescriptionRole: return s.description;
    default:              return {};
    }
}

QHash<int, QByteArray> ServerListModel::roleNames() const
{
    return {
        {IdRole,          "serverId"},
        {NameRole,        "name"},
        {HostRole,        "host"},
        {PortRole,        "port"},
        {UsernameRole,    "username"},
        {KeyAuthRole,     "keyAuth"},
        {GroupRole,       "group"},
        {DescriptionRole, "description"},
    };
}

void ServerListModel::setServers(const QList<Server> &servers)
{
    beginResetModel();
    m_servers = servers;
    endResetModel();
}

void ServerListModel::addServer(const Server &server)
{
    beginInsertRows({}, m_servers.size(), m_servers.size());
    m_servers.append(server);
    endInsertRows();
}

void ServerListModel::updateServer(const Server &server)
{
    for (int i = 0; i < m_servers.size(); ++i) {
        if (m_servers[i].id == server.id) {
            m_servers[i] = server;
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx);
            return;
        }
    }
}

void ServerListModel::removeServer(const QString &id)
{
    for (int i = 0; i < m_servers.size(); ++i) {
        if (m_servers[i].id == id) {
            beginRemoveRows({}, i, i);
            m_servers.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}

Server ServerListModel::serverAt(int row) const
{
    if (row < 0 || row >= m_servers.size()) return {};
    return m_servers.at(row);
}

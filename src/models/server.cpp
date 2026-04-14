#include "server.h"
#include <QJsonValue>
#include <QUuid>

QJsonObject Server::toJson() const
{
    return {
        {"id",             id},
        {"name",           name},
        {"host",           host},
        {"port",           port},
        {"username",       username},
        {"sshKeyPath",     sshKeyPath},
        {"keyAuthEnabled", keyAuthEnabled},
        {"description",    description},
        {"addedAt",        addedAt.toString(Qt::ISODate)},
        {"group",          group},
    };
}

Server Server::fromJson(const QJsonObject &obj)
{
    Server s;
    s.id             = obj["id"].toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
    s.name           = obj["name"].toString();
    s.host           = obj["host"].toString();
    s.port           = obj["port"].toInt(22);
    s.username       = obj["username"].toString();
    s.sshKeyPath     = obj["sshKeyPath"].toString();
    s.keyAuthEnabled = obj["keyAuthEnabled"].toBool(false);
    s.description    = obj["description"].toString();
    s.addedAt        = QDateTime::fromString(obj["addedAt"].toString(), Qt::ISODate);
    s.group          = obj["group"].toString();
    return s;
}

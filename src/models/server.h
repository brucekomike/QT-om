#pragma once
#include <QString>
#include <QDateTime>
#include <QJsonObject>

struct Server {
    QString id;
    QString name;
    QString host;
    int     port        = 22;
    QString username;
    QString sshKeyPath;
    bool    keyAuthEnabled = false;
    QString description;
    QDateTime addedAt;
    QString group;

    QJsonObject toJson() const;
    static Server fromJson(const QJsonObject &obj);
    bool isValid() const { return !host.isEmpty() && !username.isEmpty(); }
};

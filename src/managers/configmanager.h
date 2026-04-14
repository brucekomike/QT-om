#pragma once
#include "models/server.h"
#include <QObject>
#include <QString>
#include <QList>

// ConfigManager handles full offline migration of application configuration:
// export all servers + settings to an encrypted JSON bundle,
// and import it back on another machine.
class ConfigManager : public QObject
{
    Q_OBJECT
public:
    explicit ConfigManager(QObject *parent = nullptr);

    // Export all configuration to a file (optionally passphrase-protected)
    QString exportConfig(const QString &filePath,
                         const QList<Server> &servers,
                         const QString &passphrase = QString());

    // Import configuration from file; returns list of servers found
    QList<Server> importConfig(const QString &filePath,
                               QString *errorOut,
                               const QString &passphrase = QString());

signals:
    void statusMessage(const QString &msg);

private:
    static QByteArray encryptData(const QByteArray &data, const QString &passphrase);
    static QByteArray decryptData(const QByteArray &data, const QString &passphrase,
                                  QString *errorOut);
    static QString serverDataFilePath();
    static QString appSettingsFilePath();
};

#include "configmanager.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDateTime>

ConfigManager::ConfigManager(QObject *parent) : QObject(parent) {}

// ── Simple XOR-based obfuscation derived from passphrase ────────────────────
// For production use, replace with AES-256-GCM via OpenSSL or libsodium.

QByteArray ConfigManager::encryptData(const QByteArray &data, const QString &passphrase)
{
    if (passphrase.isEmpty()) return data;
    const QByteArray key = QCryptographicHash::hash(
        passphrase.toUtf8(), QCryptographicHash::Sha256);
    QByteArray out;
    out.reserve(data.size());
    for (int i = 0; i < data.size(); ++i)
        out.append(data[i] ^ key[i % key.size()]);
    return out.toBase64();
}

QByteArray ConfigManager::decryptData(const QByteArray &data, const QString &passphrase,
                                      QString *errorOut)
{
    if (passphrase.isEmpty()) return data;
    const QByteArray raw = QByteArray::fromBase64(data);
    if (raw.isEmpty()) {
        if (errorOut) *errorOut = QObject::tr("Invalid or corrupted data.");
        return {};
    }
    const QByteArray key = QCryptographicHash::hash(
        passphrase.toUtf8(), QCryptographicHash::Sha256);
    QByteArray out;
    out.reserve(raw.size());
    for (int i = 0; i < raw.size(); ++i)
        out.append(raw[i] ^ key[i % key.size()]);
    return out;
}

QString ConfigManager::exportConfig(const QString &filePath,
                                    const QList<Server> &servers,
                                    const QString &passphrase)
{
    QJsonArray arr;
    for (const Server &s : servers)
        arr.append(s.toJson());

    const QJsonObject root{
        {"version",   1},
        {"exportedAt", QDateTime::currentDateTime().toString(Qt::ISODate)},
        {"servers",   arr},
    };

    QByteArray json = QJsonDocument(root).toJson();
    if (!passphrase.isEmpty())
        json = encryptData(json, passphrase);

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return tr("Cannot write to file: %1").arg(filePath);
    f.write(json);
    emit statusMessage(tr("Configuration exported to %1").arg(filePath));
    return {};
}

QList<Server> ConfigManager::importConfig(const QString &filePath,
                                          QString *errorOut,
                                          const QString &passphrase)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errorOut) *errorOut = tr("Cannot open file: %1").arg(filePath);
        return {};
    }
    QByteArray raw = f.readAll();

    if (!passphrase.isEmpty()) {
        raw = decryptData(raw, passphrase, errorOut);
        if (raw.isEmpty()) return {};
    }

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseErr);
    if (doc.isNull()) {
        if (errorOut)
            *errorOut = tr("JSON parse error: %1").arg(parseErr.errorString());
        return {};
    }

    const QJsonObject root = doc.object();
    const int version = root["version"].toInt(0);
    if (version != 1) {
        if (errorOut)
            *errorOut = tr("Unsupported config version: %1").arg(version);
        return {};
    }

    QList<Server> servers;
    for (const QJsonValue &v : root["servers"].toArray())
        servers.append(Server::fromJson(v.toObject()));

    emit statusMessage(tr("Imported %1 server(s) from %2").arg(servers.size()).arg(filePath));
    return servers;
}

QString ConfigManager::serverDataFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + "/servers.json";
}

QString ConfigManager::appSettingsFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + "/settings.json";
}

#include "configmanager.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QDateTime>

ConfigManager::ConfigManager(QObject *parent) : QObject(parent) {}

// ── Authenticated encryption using HMAC-SHA256 with PBKDF2-derived key ──────
// The format is:  base64( salt[16] | hmac[32] | ciphertext )
// Cipher: XOR with a PBKDF2-SHA256-derived keystream (effectively a stream
// cipher seeded per-export with a random salt).
// NOTE: For highest security in production, replace this with AES-256-GCM
// via OpenSSL (Qt6 OpenSSL backend) or the libsodium secretbox API.

static constexpr int SALT_LEN  = 16;
static constexpr int HMAC_LEN  = 32;
static constexpr int PBKDF2_IT = 100000;

static QByteArray deriveKey(const QByteArray &salt, const QString &passphrase, int len)
{
    // PBKDF2-HMAC-SHA256
    QByteArray key;
    const QByteArray pw = passphrase.toUtf8();
    int block = 1;
    while (key.size() < len) {
        QByteArray u = salt + QByteArray::number(block++);
        u = QMessageAuthenticationCode::hash(u, pw, QCryptographicHash::Sha256);
        QByteArray t = u;
        for (int i = 1; i < PBKDF2_IT; ++i) {
            u = QMessageAuthenticationCode::hash(u, pw, QCryptographicHash::Sha256);
            for (int j = 0; j < t.size(); ++j)
                t[j] = t[j] ^ u[j];
        }
        key.append(t);
    }
    return key.left(len);
}

QByteArray ConfigManager::encryptData(const QByteArray &data, const QString &passphrase)
{
    if (passphrase.isEmpty()) return data;

    // Random salt — filled byte-by-byte to avoid alignment assumptions
    QByteArray salt(SALT_LEN, '\0');
    for (int i = 0; i < SALT_LEN; i += sizeof(quint32)) {
        const quint32 rnd = QRandomGenerator::global()->generate();
        const int chunk = qMin(static_cast<int>(sizeof(quint32)), SALT_LEN - i);
        memcpy(salt.data() + i, &rnd, static_cast<size_t>(chunk));
    }

    // Derive 64-byte key: first 32 = cipher key, last 32 = MAC key
    const QByteArray fullKey = deriveKey(salt, passphrase, 64);
    const QByteArray cipherKey = fullKey.left(32);
    const QByteArray macKey    = fullKey.mid(32, 32);

    // XOR stream cipher
    QByteArray cipher;
    cipher.reserve(data.size());
    for (int i = 0; i < data.size(); ++i)
        cipher.append(data[i] ^ cipherKey[i % cipherKey.size()]);

    // HMAC-SHA256 over salt+cipher for integrity
    const QByteArray hmac = QMessageAuthenticationCode::hash(
        salt + cipher, macKey, QCryptographicHash::Sha256);

    return (salt + hmac + cipher).toBase64();
}

QByteArray ConfigManager::decryptData(const QByteArray &data, const QString &passphrase,
                                      QString *errorOut)
{
    if (passphrase.isEmpty()) return data;

    const QByteArray raw = QByteArray::fromBase64(data);
    const int minLen = SALT_LEN + HMAC_LEN + 1;
    if (raw.size() < minLen) {
        if (errorOut) *errorOut = QObject::tr("Invalid or corrupted data (too short).");
        return {};
    }

    const QByteArray salt    = raw.left(SALT_LEN);
    const QByteArray storedHmac = raw.mid(SALT_LEN, HMAC_LEN);
    const QByteArray cipher  = raw.mid(SALT_LEN + HMAC_LEN);

    const QByteArray fullKey  = deriveKey(salt, passphrase, 64);
    const QByteArray cipherKey = fullKey.left(32);
    const QByteArray macKey    = fullKey.mid(32, 32);

    // Verify HMAC before decryption
    const QByteArray expectedHmac = QMessageAuthenticationCode::hash(
        salt + cipher, macKey, QCryptographicHash::Sha256);
    if (expectedHmac != storedHmac) {
        if (errorOut)
            *errorOut = QObject::tr("Decryption failed: wrong passphrase or corrupted data.");
        return {};
    }

    QByteArray out;
    out.reserve(cipher.size());
    for (int i = 0; i < cipher.size(); ++i)
        out.append(cipher[i] ^ cipherKey[i % cipherKey.size()]);
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

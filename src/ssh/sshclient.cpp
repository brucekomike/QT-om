#include "sshclient.h"
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QUuid>

SshClient::SshClient(QObject *parent) : QObject(parent) {}

QStringList SshClient::commonSshArgs(const QString &keyPath, int port,
                                     bool strictHostKeyChecking)
{
    QStringList args;
    args << "-p" << QString::number(port);
    if (!keyPath.isEmpty())
        args << "-i" << keyPath;
    args << "-o" << "BatchMode=yes";
    args << "-o" << "ConnectTimeout=10";
    if (!strictHostKeyChecking)
        args << "-o" << "StrictHostKeyChecking=no";
    return args;
}

SshClient::Result SshClient::runProcess(const QString &program, const QStringList &args,
                                        const QString &stdinData, int timeoutMs)
{
    QProcess proc;
    proc.setProgram(program);
    proc.setArguments(args);
    proc.start();

    if (!stdinData.isEmpty()) {
        proc.write(stdinData.toUtf8());
        proc.closeWriteChannel();
    }

    bool finished = proc.waitForFinished(timeoutMs);
    Result r;
    r.output      = QString::fromUtf8(proc.readAllStandardOutput());
    r.errorOutput = QString::fromUtf8(proc.readAllStandardError());
    r.exitCode    = proc.exitCode();
    r.success     = finished && (proc.exitStatus() == QProcess::NormalExit) && (r.exitCode == 0);

    for (const QString &line : r.output.split('\n'))
        if (!line.trimmed().isEmpty())
            emit outputLine(line);

    return r;
}

SshClient::Result SshClient::runCommand(const QString &host, int port,
                                        const QString &username, const QString &keyPath,
                                        const QString &command)
{
    QStringList args = commonSshArgs(keyPath, port);
    args << QString("%1@%2").arg(username, host) << command;
    return runProcess("ssh", args, {}, 60000);
}

SshClient::Result SshClient::runCommandWithPassword(const QString &host, int port,
                                                    const QString &username,
                                                    const QString &password,
                                                    const QString &command)
{
    // Use sshpass if available, otherwise fall back to expect approach
    QStringList args;
    args << "-p" << password;
    args << "ssh";
    args << "-p" << QString::number(port);
    args << "-o" << "StrictHostKeyChecking=no";
    args << "-o" << "ConnectTimeout=10";
    args << QString("%1@%2").arg(username, host);
    args << command;
    return runProcess("sshpass", args, {}, 60000);
}

QString SshClient::generateKeyPair(const QString &comment)
{
    const QString keyDir  = QDir::homePath() + "/.ssh";
    QDir().mkpath(keyDir);

    const QString keyBase = keyDir + "/qt-om_" +
                            QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    const QString keyPath = keyBase;
    const QString pubPath = keyBase + ".pub";

    QStringList args;
    args << "-t" << "ed25519"
         << "-f" << keyPath
         << "-N" << ""       // no passphrase
         << "-C" << (comment.isEmpty() ? "qt-om" : comment);

    QProcess proc;
    proc.start("ssh-keygen", args);
    proc.waitForFinished(15000);

    if (proc.exitCode() == 0 && QFileInfo::exists(pubPath))
        return keyPath;
    return {};
}

SshClient::Result SshClient::copyPublicKey(const QString &host, int port,
                                           const QString &username,
                                           const QString &password,
                                           const QString &publicKeyPath)
{
    // Read the public key content
    QFile pubFile(publicKeyPath);
    if (!pubFile.open(QIODevice::ReadOnly))
        return {false, {}, "Cannot open public key file: " + publicKeyPath, -1};
    const QString pubKey = QString::fromUtf8(pubFile.readAll()).trimmed();

    // Remote command: append key to authorized_keys
    const QString cmd = QString(
        "mkdir -p ~/.ssh && "
        "chmod 700 ~/.ssh && "
        "echo '%1' >> ~/.ssh/authorized_keys && "
        "chmod 600 ~/.ssh/authorized_keys"
    ).arg(pubKey);

    return runCommandWithPassword(host, port, username, password, cmd);
}

SshClient::Result SshClient::downloadFile(const QString &host, int port,
                                          const QString &username,
                                          const QString &keyPath,
                                          const QString &remotePath,
                                          const QString &localPath)
{
    QStringList args;
    args << "-P" << QString::number(port);
    if (!keyPath.isEmpty())
        args << "-i" << keyPath;
    args << "-o" << "StrictHostKeyChecking=no"
         << "-o" << "BatchMode=yes"
         << "-o" << "ConnectTimeout=10"
         << QString("%1@%2:%3").arg(username, host, remotePath)
         << localPath;
    return runProcess("scp", args, {}, 300000); // 5 min timeout for large files
}

SshClient::Result SshClient::uploadFile(const QString &host, int port,
                                        const QString &username, const QString &keyPath,
                                        const QString &localPath,
                                        const QString &remotePath)
{
    QStringList args;
    args << "-P" << QString::number(port);
    if (!keyPath.isEmpty())
        args << "-i" << keyPath;
    args << "-o" << "StrictHostKeyChecking=no"
         << "-o" << "BatchMode=yes"
         << "-o" << "ConnectTimeout=10"
         << localPath
         << QString("%1@%2:%3").arg(username, host, remotePath);
    return runProcess("scp", args, {}, 300000);
}

bool SshClient::testConnection(const QString &host, int port,
                               const QString &username, const QString &keyPath)
{
    const Result r = runCommand(host, port, username, keyPath, "echo OK");
    return r.success && r.output.contains("OK");
}

QString SshClient::defaultPrivateKeyPath()
{
    const QString sshDir = QDir::homePath() + "/.ssh";
    for (const QString &name : {"id_ed25519", "id_rsa", "id_ecdsa"}) {
        const QString path = sshDir + "/" + name;
        if (QFileInfo::exists(path)) return path;
    }
    return {};
}

QString SshClient::defaultPublicKeyPath()
{
    const QString priv = defaultPrivateKeyPath();
    if (!priv.isEmpty()) return priv + ".pub";
    return {};
}

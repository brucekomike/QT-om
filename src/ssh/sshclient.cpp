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
    // Use sshpass with -e (read password from SSHPASS env var) instead of -p
    // to avoid exposing the password in the process argument list (visible in ps).
    QStringList args;
    args << "-e";   // read password from environment variable SSHPASS
    args << "ssh";
    args << "-p" << QString::number(port);
    args << "-o" << "StrictHostKeyChecking=no";
    args << "-o" << "ConnectTimeout=10";
    args << QString("%1@%2").arg(username, host);
    args << command;

    QProcess proc;
    proc.setProgram("sshpass");
    proc.setArguments(args);
    // Inject password via environment only — not visible in the argument list
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("SSHPASS", password);
    proc.setProcessEnvironment(env);
    proc.start();
    bool finished = proc.waitForFinished(60000);

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

    // Pipe the key via stdin to avoid shell injection from key content.
    // The remote command reads one line from stdin and appends it safely.
    const QString cmd =
        "mkdir -p ~/.ssh && "
        "chmod 700 ~/.ssh && "
        "read -r _k && printf '%s\\n' \"$_k\" >> ~/.ssh/authorized_keys && "
        "chmod 600 ~/.ssh/authorized_keys";

    // Use SSHPASS env var to avoid exposing password in argument list
    QStringList args;
    args << "-e";   // read password from SSHPASS environment variable
    args << "ssh";
    args << "-p" << QString::number(port);
    args << "-o" << "StrictHostKeyChecking=no";
    args << "-o" << "ConnectTimeout=10";
    args << QString("%1@%2").arg(username, host);
    args << "bash" << "-c" << cmd;

    QProcess proc;
    proc.setProgram("sshpass");
    proc.setArguments(args);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("SSHPASS", password);
    proc.setProcessEnvironment(env);
    proc.start();
    proc.write((pubKey + "\n").toUtf8());
    proc.closeWriteChannel();
    bool finished = proc.waitForFinished(60000);

    Result r;
    r.output      = QString::fromUtf8(proc.readAllStandardOutput());
    r.errorOutput = QString::fromUtf8(proc.readAllStandardError());
    r.exitCode    = proc.exitCode();
    r.success     = finished && (proc.exitStatus() == QProcess::NormalExit) && (r.exitCode == 0);
    return r;
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

#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <functional>

// SshClient wraps OpenSSH command-line tools (ssh, scp, ssh-keygen, ssh-copy-id)
// to provide SSH operations without requiring a C-level libssh dependency.
class SshClient : public QObject
{
    Q_OBJECT
public:
    struct Result {
        bool    success = false;
        QString output;
        QString errorOutput;
        int     exitCode = -1;
    };

    explicit SshClient(QObject *parent = nullptr);

    // Build common SSH options (key file, port, known-host bypass for first connect)
    static QStringList commonSshArgs(const QString &keyPath, int port,
                                     bool strictHostKeyChecking = false);

    // Run a single command on a remote host and return result (blocking)
    Result runCommand(const QString &host, int port, const QString &username,
                      const QString &keyPath, const QString &command);

    // Run command with password via sshpass (blocking)
    Result runCommandWithPassword(const QString &host, int port, const QString &username,
                                  const QString &password, const QString &command);

    // Generate a new SSH key pair; returns path of private key
    static QString generateKeyPair(const QString &comment = QString());

    // Copy public key to remote using password authentication (like ssh-copy-id)
    Result copyPublicKey(const QString &host, int port, const QString &username,
                         const QString &password, const QString &publicKeyPath);

    // Download remote file to local path via scp
    Result downloadFile(const QString &host, int port, const QString &username,
                        const QString &keyPath, const QString &remotePath,
                        const QString &localPath);

    // Upload local file to remote via scp
    Result uploadFile(const QString &host, int port, const QString &username,
                      const QString &keyPath, const QString &localPath,
                      const QString &remotePath);

    // Test connectivity (returns true if SSH login succeeds)
    bool testConnection(const QString &host, int port, const QString &username,
                        const QString &keyPath);

    // Default SSH key paths
    static QString defaultPrivateKeyPath();
    static QString defaultPublicKeyPath();

signals:
    void outputLine(const QString &line);

private:
    Result runProcess(const QString &program, const QStringList &args,
                      const QString &stdinData = QString(), int timeoutMs = 30000);
};

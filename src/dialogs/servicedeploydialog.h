#pragma once
#include "managers/servicemanager.h"
#include "models/server.h"
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

class ServiceDeployDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ServiceDeployDialog(const Server &server, QWidget *parent = nullptr);

    ServiceManager::DeployConfig deployConfig() const;

private slots:
    void onDeployClicked();

private:
    void setupUi();

    Server      m_server;
    QLineEdit  *m_repoEdit;
    QLineEdit  *m_branchEdit;
    QLineEdit  *m_pathEdit;
    QLineEdit  *m_scriptEdit;
    QListWidget *m_envList;
    QPushButton *m_addEnvBtn;
    QPushButton *m_removeEnvBtn;
    QPushButton *m_deployBtn;
    QTextEdit  *m_logEdit;
    QLabel     *m_statusLabel;
};

#pragma once
#include "models/server.h"
#include "managers/healthcheckmanager.h"
#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>

class HealthPanel : public QWidget
{
    Q_OBJECT
public:
    explicit HealthPanel(QWidget *parent = nullptr);

    void setServers(const QList<Server> &servers);

private slots:
    void onCheckAll();
    void onStatusReceived(const HealthStatus &status);
    void onAutoCheckToggled(bool enabled);

private:
    int rowForServer(const QString &serverId) const;
    void ensureRow(const QString &serverId, const QString &name);

    QList<Server>       m_servers;
    QTableWidget       *m_table;
    QPushButton        *m_checkBtn;
    QPushButton        *m_autoBtn;
    QLabel             *m_lastCheckLabel;
    QTimer             *m_timer;
    HealthCheckManager *m_mgr;
};

#pragma once
#include "models/server.h"
#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>

class ServicesPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ServicesPanel(QWidget *parent = nullptr);

public slots:
    void setCurrentServer(const Server &server);

private slots:
    void onDeploy();
    void onRefreshList();

private:
    Server       m_server;
    QLabel      *m_serverLabel;
    QListWidget *m_serviceList;
    QPushButton *m_deployBtn;
    QPushButton *m_refreshBtn;
    QTextEdit   *m_logEdit;
};

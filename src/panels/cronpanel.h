#pragma once
#include "models/server.h"
#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QTextEdit>

class CronPanel : public QWidget
{
    Q_OBJECT
public:
    explicit CronPanel(QWidget *parent = nullptr);

public slots:
    void setCurrentServer(const Server &server);

private slots:
    void onRefresh();
    void onAddJob();
    void onEditJob();
    void onRemoveJob();

private:
    Server        m_server;
    QLabel       *m_serverLabel;
    QTableWidget *m_table;
    QPushButton  *m_addBtn;
    QPushButton  *m_editBtn;
    QPushButton  *m_removeBtn;
    QPushButton  *m_refreshBtn;
    QTextEdit    *m_logEdit;
};

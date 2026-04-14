#pragma once
#include "models/server.h"
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QCheckBox>

class BackupPanel : public QWidget
{
    Q_OBJECT
public:
    explicit BackupPanel(QWidget *parent = nullptr);

public slots:
    void setCurrentServer(const Server &server);

private slots:
    void onRunBackup();
    void onSchedule();
    void onUnschedule();
    void onRefreshLocalBackups();

private:
    Server      m_server;
    QLabel     *m_serverLabel;
    QLineEdit  *m_scriptEdit;
    QLineEdit  *m_archiveEdit;
    QLineEdit  *m_localDirEdit;
    QLineEdit  *m_cronEdit;
    QLineEdit  *m_commentEdit;
    QListWidget *m_backupList;
    QPushButton *m_runBtn;
    QPushButton *m_scheduleBtn;
    QPushButton *m_unscheduleBtn;
    QTextEdit   *m_logEdit;
};

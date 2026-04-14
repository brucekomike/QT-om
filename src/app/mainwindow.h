#pragma once

#ifdef USE_KF6
#  include <KXmlGuiWindow>
using MainWindowBase = KXmlGuiWindow;
#else
#  include <QMainWindow>
using MainWindowBase = QMainWindow;
#endif

#include "managers/servermanager.h"
#include "panels/serverspanel.h"
#include "panels/servicespanel.h"
#include "panels/cronpanel.h"
#include "panels/backuppanel.h"
#include "panels/healthpanel.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>

class MainWindow : public MainWindowBase
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onNavigationChanged(int index);
    void onServerSelected(const Server &server);
    void onImportExportConfig();

private:
    void setupUi();
    void setupActions();
    void setupStatusBar();

    ServerManager  *m_serverMgr;

    QListWidget    *m_navList;
    QStackedWidget *m_stack;

    ServersPanel   *m_serversPanel;
    ServicesPanel  *m_servicesPanel;
    CronPanel      *m_cronPanel;
    BackupPanel    *m_backupPanel;
    HealthPanel    *m_healthPanel;

    QLabel         *m_statusLabel;
    Server          m_currentServer;
};

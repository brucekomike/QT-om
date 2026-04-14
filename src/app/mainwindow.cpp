#include "mainwindow.h"
#include "dialogs/configmigrationdialog.h"
#include <QApplication>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QAction>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : MainWindowBase(parent)
    , m_serverMgr(new ServerManager(this))
{
    setupUi();
    setupActions();
    setupStatusBar();

    // Propagate server list to health panel
    m_healthPanel->setServers(m_serverMgr->servers());
    connect(m_serverMgr, &ServerManager::serverAdded, this, [this](const Server &){
        m_healthPanel->setServers(m_serverMgr->servers());
    });
    connect(m_serverMgr, &ServerManager::serverRemoved, this, [this](const QString &){
        m_healthPanel->setServers(m_serverMgr->servers());
    });
    connect(m_serverMgr, &ServerManager::statusMessage,
            m_statusLabel, &QLabel::setText);
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("QT-om — DevOps Server Manager"));
    setMinimumSize(1024, 640);

    // Navigation sidebar
    m_navList = new QListWidget(this);
    m_navList->setFixedWidth(160);
    m_navList->addItem(tr("Servers"));
    m_navList->addItem(tr("Services"));
    m_navList->addItem(tr("Cron Jobs"));
    m_navList->addItem(tr("Backups"));
    m_navList->addItem(tr("Health"));
    m_navList->setCurrentRow(0);

    // Panels
    m_serversPanel  = new ServersPanel(m_serverMgr, this);
    m_servicesPanel = new ServicesPanel(this);
    m_cronPanel     = new CronPanel(this);
    m_backupPanel   = new BackupPanel(this);
    m_healthPanel   = new HealthPanel(this);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_serversPanel);
    m_stack->addWidget(m_servicesPanel);
    m_stack->addWidget(m_cronPanel);
    m_stack->addWidget(m_backupPanel);
    m_stack->addWidget(m_healthPanel);

    connect(m_navList, &QListWidget::currentRowChanged,
            this, &MainWindow::onNavigationChanged);
    connect(m_serversPanel, &ServersPanel::serverSelected,
            this, &MainWindow::onServerSelected);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_navList);
    splitter->addWidget(m_stack);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);
}

void MainWindow::setupActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *importExportAct = new QAction(tr("Import / Export Config…"), this);
    importExportAct->setShortcut(QKeySequence("Ctrl+Shift+E"));
    connect(importExportAct, &QAction::triggered, this, &MainWindow::onImportExportConfig);
    fileMenu->addAction(importExportAct);
    fileMenu->addSeparator();

    QAction *quitAct = new QAction(tr("Quit"), this);
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(quitAct);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAct = new QAction(tr("About QT-om"), this);
    connect(aboutAct, &QAction::triggered, this, [this]{
        QMessageBox::about(this, tr("About QT-om"),
            tr("<b>QT-om</b> v0.1.0<br>"
               "A cross-platform Qt DevOps server manager.<br><br>"
               "Features:<br>"
               "• Add servers with SSH key authentication setup<br>"
               "• Deploy services from Git repositories<br>"
               "• Manage remote cron jobs<br>"
               "• Automated backups with remote script execution<br>"
               "• Health check dashboard<br>"
               "• Full config offline migration"));
    });
    helpMenu->addAction(aboutAct);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr("Ready."), this);
    statusBar()->addWidget(m_statusLabel, 1);
}

void MainWindow::onNavigationChanged(int index)
{
    m_stack->setCurrentIndex(index);
}

void MainWindow::onServerSelected(const Server &server)
{
    m_currentServer = server;
    m_servicesPanel->setCurrentServer(server);
    m_cronPanel->setCurrentServer(server);
    m_backupPanel->setCurrentServer(server);
}

void MainWindow::onImportExportConfig()
{
    ConfigMigrationDialog dlg(m_serverMgr->servers(), this);
    if (dlg.exec() == QDialog::Accepted) {
        for (const Server &s : dlg.importedServers())
            m_serverMgr->addServer(s);
    }
}

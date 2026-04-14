#include "servicespanel.h"
#include "dialogs/servicedeploydialog.h"
#include "managers/servicemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

ServicesPanel::ServicesPanel(QWidget *parent) : QWidget(parent)
{
    m_serverLabel = new QLabel(tr("No server selected."), this);
    m_serviceList = new QListWidget(this);
    m_deployBtn   = new QPushButton(tr("Deploy Service…"), this);
    m_refreshBtn  = new QPushButton(tr("Refresh List"), this);
    m_logEdit     = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(120);

    connect(m_deployBtn,  &QPushButton::clicked, this, &ServicesPanel::onDeploy);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ServicesPanel::onRefreshList);

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(m_deployBtn);
    btnRow->addWidget(m_refreshBtn);
    btnRow->addStretch();

    auto *vbox = new QVBoxLayout(this);
    vbox->addWidget(m_serverLabel);
    vbox->addLayout(btnRow);
    vbox->addWidget(m_serviceList, 1);
    vbox->addWidget(m_logEdit);
}

void ServicesPanel::setCurrentServer(const Server &server)
{
    m_server = server;
    m_serverLabel->setText(tr("Server: <b>%1</b> (%2)")
        .arg(server.name.isEmpty() ? server.host : server.name)
        .arg(server.host));
    onRefreshList();
}

void ServicesPanel::onDeploy()
{
    if (!m_server.isValid()) {
        QMessageBox::information(this, tr("No Server"), tr("Please select a server first."));
        return;
    }
    ServiceDeployDialog dlg(m_server, this);
    dlg.exec();
    onRefreshList();
}

void ServicesPanel::onRefreshList()
{
    m_serviceList->clear();
    if (!m_server.isValid()) return;

    ServiceManager mgr(this);
    connect(&mgr, &ServiceManager::progressLine, m_logEdit, &QTextEdit::append);
    const QStringList services = mgr.listInstalledServices(m_server);
    m_serviceList->addItems(services);
    if (services.isEmpty())
        m_serviceList->addItem(tr("(no services found in ~/apps/)"));
}

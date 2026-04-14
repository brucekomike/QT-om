#include "healthpanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDateTime>

enum Col { ColStatus=0, ColName, ColHost, ColCpu, ColMem, ColDisk, ColUptime, ColChecked };

HealthPanel::HealthPanel(QWidget *parent) : QWidget(parent)
{
    m_mgr   = new HealthCheckManager(this);
    m_timer = new QTimer(this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        tr("Status"), tr("Name"), tr("Host"),
        tr("CPU %"), tr("RAM (MB)"), tr("Disk (GB)"),
        tr("Uptime"), tr("Last Check")
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_checkBtn       = new QPushButton(tr("Check All Now"), this);
    m_autoBtn        = new QPushButton(tr("Auto (60s)"), this);
    m_autoBtn->setCheckable(true);
    m_lastCheckLabel = new QLabel(tr("Not checked yet."), this);

    connect(m_checkBtn, &QPushButton::clicked, this, &HealthPanel::onCheckAll);
    connect(m_autoBtn,  &QPushButton::toggled, this, &HealthPanel::onAutoCheckToggled);
    connect(m_mgr, &HealthCheckManager::healthChecked,
            this, &HealthPanel::onStatusReceived);
    connect(m_timer, &QTimer::timeout, this, &HealthPanel::onCheckAll);

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(m_checkBtn);
    btnRow->addWidget(m_autoBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_lastCheckLabel);

    auto *vbox = new QVBoxLayout(this);
    vbox->addLayout(btnRow);
    vbox->addWidget(m_table, 1);
}

void HealthPanel::setServers(const QList<Server> &servers)
{
    m_servers = servers;
    // Ensure a row exists for every server
    for (const Server &s : servers)
        ensureRow(s.id, s.name.isEmpty() ? s.host : s.name);
}

void HealthPanel::onCheckAll()
{
    m_checkBtn->setEnabled(false);
    m_mgr->checkAll(m_servers);
    m_lastCheckLabel->setText(tr("Last check: %1")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm")));
    m_checkBtn->setEnabled(true);
}

void HealthPanel::onAutoCheckToggled(bool enabled)
{
    if (enabled) {
        m_timer->start(60000);
        m_autoBtn->setText(tr("Auto ON (60s)"));
        onCheckAll();
    } else {
        m_timer->stop();
        m_autoBtn->setText(tr("Auto (60s)"));
    }
}

void HealthPanel::onStatusReceived(const HealthStatus &s)
{
    ensureRow(s.serverId, s.serverName);
    const int row = rowForServer(s.serverId);
    if (row < 0) return;

    auto setCell = [&](int col, const QString &text) {
        if (!m_table->item(row, col))
            m_table->setItem(row, col, new QTableWidgetItem(text));
        else
            m_table->item(row, col)->setText(text);
    };

    const QString statusIcon = s.reachable ? "●" : "○";
    setCell(ColStatus,  statusIcon);
    m_table->item(row, ColStatus)->setForeground(
        s.reachable ? Qt::green : Qt::red);
    setCell(ColName,    s.serverName);
    setCell(ColCpu,     s.reachable ? QString::number(s.cpuPercent, 'f', 1) : "—");
    setCell(ColMem,     s.reachable
        ? QString("%1 / %2").arg(s.memUsedMb, 0, 'f', 0).arg(s.memTotalMb, 0, 'f', 0)
        : "—");
    setCell(ColDisk,    s.reachable
        ? QString("%1 / %2").arg(s.diskUsedGb, 0, 'f', 1).arg(s.diskTotalGb, 0, 'f', 1)
        : "—");
    setCell(ColUptime,  s.reachable ? s.uptimeStr : s.errorMessage);
    setCell(ColChecked, s.checkedAt.toString("yyyy-MM-dd HH:mm"));
}

int HealthPanel::rowForServer(const QString &serverId) const
{
    for (int i = 0; i < m_table->rowCount(); ++i) {
        QTableWidgetItem *item = m_table->item(i, ColName);
        if (item && item->data(Qt::UserRole).toString() == serverId)
            return i;
    }
    return -1;
}

void HealthPanel::ensureRow(const QString &serverId, const QString &name)
{
    if (rowForServer(serverId) >= 0) return;
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    auto *nameItem = new QTableWidgetItem(name);
    nameItem->setData(Qt::UserRole, serverId);
    m_table->setItem(row, ColName,    nameItem);
    m_table->setItem(row, ColStatus,  new QTableWidgetItem("?"));
    m_table->setItem(row, ColCpu,     new QTableWidgetItem("—"));
    m_table->setItem(row, ColMem,     new QTableWidgetItem("—"));
    m_table->setItem(row, ColDisk,    new QTableWidgetItem("—"));
    m_table->setItem(row, ColUptime,  new QTableWidgetItem("—"));
    m_table->setItem(row, ColChecked, new QTableWidgetItem("—"));
}

#include "cronpanel.h"
#include "dialogs/cronjobeditdialog.h"
#include "managers/cronmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

CronPanel::CronPanel(QWidget *parent) : QWidget(parent)
{
    m_serverLabel = new QLabel(tr("No server selected."), this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("Schedule"), tr("Command"), tr("Comment")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_addBtn     = new QPushButton(tr("Add Job"),   this);
    m_editBtn    = new QPushButton(tr("Edit"),       this);
    m_removeBtn  = new QPushButton(tr("Remove"),     this);
    m_refreshBtn = new QPushButton(tr("Refresh"),    this);
    m_editBtn->setEnabled(false);
    m_removeBtn->setEnabled(false);

    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(100);

    connect(m_addBtn,     &QPushButton::clicked, this, &CronPanel::onAddJob);
    connect(m_editBtn,    &QPushButton::clicked, this, &CronPanel::onEditJob);
    connect(m_removeBtn,  &QPushButton::clicked, this, &CronPanel::onRemoveJob);
    connect(m_refreshBtn, &QPushButton::clicked, this, &CronPanel::onRefresh);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]{
        const bool sel = !m_table->selectedItems().isEmpty();
        m_editBtn->setEnabled(sel);
        m_removeBtn->setEnabled(sel);
    });

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_editBtn);
    btnRow->addWidget(m_removeBtn);
    btnRow->addWidget(m_refreshBtn);
    btnRow->addStretch();

    auto *vbox = new QVBoxLayout(this);
    vbox->addWidget(m_serverLabel);
    vbox->addLayout(btnRow);
    vbox->addWidget(m_table, 1);
    vbox->addWidget(m_logEdit);
}

void CronPanel::setCurrentServer(const Server &server)
{
    m_server = server;
    m_serverLabel->setText(tr("Server: <b>%1</b> (%2)")
        .arg(server.name.isEmpty() ? server.host : server.name)
        .arg(server.host));
    onRefresh();
}

void CronPanel::onRefresh()
{
    m_table->setRowCount(0);
    if (!m_server.isValid()) return;

    CronManager mgr(this);
    connect(&mgr, &CronManager::progressLine, m_logEdit, &QTextEdit::append);
    QString err;
    const QList<CronJob> jobs = mgr.listCronJobs(m_server, &err);
    if (!err.isEmpty()) {
        m_logEdit->append(tr("Error: %1").arg(err));
        return;
    }
    for (const CronJob &job : jobs) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(job.schedule));
        m_table->setItem(row, 1, new QTableWidgetItem(job.command));
        m_table->setItem(row, 2, new QTableWidgetItem(job.comment));
    }
}

void CronPanel::onAddJob()
{
    if (!m_server.isValid()) {
        QMessageBox::information(this, tr("No Server"), tr("Please select a server first."));
        return;
    }
    CronJobEditDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    CronManager mgr(this);
    connect(&mgr, &CronManager::progressLine, m_logEdit, &QTextEdit::append);
    const QString err = mgr.addOrUpdateCronJob(m_server, dlg.cronJob());
    if (!err.isEmpty())
        QMessageBox::warning(this, tr("Error"), err);
    onRefresh();
}

void CronPanel::onEditJob()
{
    const int row = m_table->currentRow();
    if (row < 0) return;

    CronJob existing;
    existing.schedule = m_table->item(row, 0)->text();
    existing.command  = m_table->item(row, 1)->text();
    existing.comment  = m_table->item(row, 2)->text();

    CronJobEditDialog dlg(existing, this);
    if (dlg.exec() != QDialog::Accepted) return;

    CronManager mgr(this);
    connect(&mgr, &CronManager::progressLine, m_logEdit, &QTextEdit::append);
    const QString err = mgr.addOrUpdateCronJob(m_server, dlg.cronJob());
    if (!err.isEmpty())
        QMessageBox::warning(this, tr("Error"), err);
    onRefresh();
}

void CronPanel::onRemoveJob()
{
    const int row = m_table->currentRow();
    if (row < 0) return;
    const QString comment = m_table->item(row, 2)->text();
    if (QMessageBox::question(this, tr("Remove Cron Job"),
            tr("Remove cron job '%1'?").arg(comment)) != QMessageBox::Yes) return;

    CronManager mgr(this);
    connect(&mgr, &CronManager::progressLine, m_logEdit, &QTextEdit::append);
    const QString err = mgr.removeCronJob(m_server, comment);
    if (!err.isEmpty())
        QMessageBox::warning(this, tr("Error"), err);
    onRefresh();
}

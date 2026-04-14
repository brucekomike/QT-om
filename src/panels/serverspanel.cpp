#include "serverspanel.h"
#include "dialogs/addserverdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QInputDialog>
#include <QMessageBox>
#include <QGroupBox>

ServersPanel::ServersPanel(ServerManager *mgr, QWidget *parent)
    : QWidget(parent), m_mgr(mgr)
{
    m_listView   = new QListView(this);
    m_listView->setModel(mgr->model());
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);

    m_addBtn    = new QPushButton(tr("Add Server"), this);
    m_editBtn   = new QPushButton(tr("Edit"), this);
    m_removeBtn = new QPushButton(tr("Remove"), this);
    m_keyAuthBtn = new QPushButton(tr("Setup SSH Key"), this);
    m_editBtn->setEnabled(false);
    m_removeBtn->setEnabled(false);
    m_keyAuthBtn->setEnabled(false);

    m_detailLabel = new QLabel(tr("Select a server to view details."), this);
    m_detailLabel->setWordWrap(true);
    m_detailLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(120);
    m_logEdit->setPlaceholderText(tr("Activity log…"));

    connect(m_addBtn,    &QPushButton::clicked, this, &ServersPanel::onAddServer);
    connect(m_editBtn,   &QPushButton::clicked, this, &ServersPanel::onEditServer);
    connect(m_removeBtn, &QPushButton::clicked, this, &ServersPanel::onRemoveServer);
    connect(m_keyAuthBtn, &QPushButton::clicked, this, &ServersPanel::onSetupKeyAuth);
    connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ServersPanel::onSelectionChanged);
    connect(mgr, &ServerManager::statusMessage, m_logEdit, &QTextEdit::append);

    auto *btnBar = new QHBoxLayout;
    btnBar->addWidget(m_addBtn);
    btnBar->addWidget(m_editBtn);
    btnBar->addWidget(m_removeBtn);
    btnBar->addWidget(m_keyAuthBtn);
    btnBar->addStretch();

    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->addLayout(btnBar);
    leftLayout->addWidget(m_listView);

    auto *rightWidget = new QGroupBox(tr("Server Details"), this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->addWidget(m_detailLabel);
    rightLayout->addStretch();

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    auto *vbox = new QVBoxLayout(this);
    vbox->addWidget(splitter);
    vbox->addWidget(m_logEdit);
}

void ServersPanel::onAddServer()
{
    AddServerDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    Server s = dlg.server();
    m_mgr->addServer(s);

    if (dlg.setupKeyAuth()) {
        const QString err = m_mgr->setupSshKeyAuth(s.id, dlg.password());
        if (!err.isEmpty())
            QMessageBox::warning(this, tr("Key Setup Failed"), err);
    }
}

void ServersPanel::onEditServer()
{
    const QModelIndex idx = m_listView->currentIndex();
    if (!idx.isValid()) return;
    const Server s = m_mgr->model()->serverAt(idx.row());
    AddServerDialog dlg(s, this);
    if (dlg.exec() != QDialog::Accepted) return;
    Server updated = dlg.server();
    updated.id = s.id; // preserve ID
    m_mgr->updateServer(updated);
}

void ServersPanel::onRemoveServer()
{
    const QModelIndex idx = m_listView->currentIndex();
    if (!idx.isValid()) return;
    const Server s = m_mgr->model()->serverAt(idx.row());
    const auto ans = QMessageBox::question(this, tr("Remove Server"),
        tr("Remove server '%1'?").arg(s.name.isEmpty() ? s.host : s.name));
    if (ans == QMessageBox::Yes)
        m_mgr->removeServer(s.id);
}

void ServersPanel::onSetupKeyAuth()
{
    const QModelIndex idx = m_listView->currentIndex();
    if (!idx.isValid()) return;
    const Server s = m_mgr->model()->serverAt(idx.row());

    bool ok;
    const QString pw = QInputDialog::getText(
        this, tr("SSH Password"),
        tr("Enter SSH password for %1@%2 to set up key authentication:")
            .arg(s.username, s.host),
        QLineEdit::Password, {}, &ok);
    if (!ok || pw.isEmpty()) return;

    m_logEdit->append(tr("Setting up SSH key authentication for %1…").arg(s.host));
    const QString err = m_mgr->setupSshKeyAuth(s.id, pw);
    if (!err.isEmpty())
        QMessageBox::warning(this, tr("Key Setup Failed"), err);
    else
        QMessageBox::information(this, tr("Success"),
            tr("SSH key authentication configured for %1.").arg(s.host));
}

void ServersPanel::onSelectionChanged()
{
    const bool hasSelection = m_listView->currentIndex().isValid();
    m_editBtn->setEnabled(hasSelection);
    m_removeBtn->setEnabled(hasSelection);
    m_keyAuthBtn->setEnabled(hasSelection);

    if (hasSelection) {
        const Server s = m_mgr->model()->serverAt(m_listView->currentIndex().row());
        refreshDetails(s);
        emit serverSelected(s);
    }
}

void ServersPanel::refreshDetails(const Server &s)
{
    m_detailLabel->setText(
        tr("<b>%1</b><br>"
           "Host: %2:%3<br>"
           "User: %4<br>"
           "Key Auth: %5<br>"
           "Group: %6<br>"
           "Added: %7<br>"
           "<br>%8")
            .arg(s.name.isEmpty() ? s.host : s.name)
            .arg(s.host)
            .arg(s.port)
            .arg(s.username)
            .arg(s.keyAuthEnabled ? tr("Yes") : tr("No"))
            .arg(s.group.isEmpty() ? tr("(none)") : s.group)
            .arg(s.addedAt.toString("yyyy-MM-dd HH:mm"))
            .arg(s.description));
}

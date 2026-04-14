#include "servicedeploydialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QMessageBox>

ServiceDeployDialog::ServiceDeployDialog(const Server &server, QWidget *parent)
    : QDialog(parent), m_server(server)
{
    setupUi();
    setWindowTitle(tr("Deploy Service — %1").arg(server.name.isEmpty() ? server.host : server.name));
}

void ServiceDeployDialog::setupUi()
{
    setMinimumSize(560, 520);

    m_repoEdit   = new QLineEdit(this);
    m_repoEdit->setPlaceholderText("https://github.com/user/myapp.git");
    m_branchEdit = new QLineEdit("main", this);
    m_pathEdit   = new QLineEdit("~/apps/myapp", this);
    m_scriptEdit = new QLineEdit("deploy/deploy.sh", this);
    m_envList    = new QListWidget(this);
    m_envList->setMaximumHeight(120);
    m_addEnvBtn    = new QPushButton(tr("Add Env Var"), this);
    m_removeEnvBtn = new QPushButton(tr("Remove"), this);
    m_deployBtn    = new QPushButton(tr("▶ Deploy"), this);
    m_deployBtn->setDefault(true);
    m_logEdit    = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_statusLabel = new QLabel(this);

    connect(m_addEnvBtn, &QPushButton::clicked, this, [this]{
        bool ok;
        const QString kv = QInputDialog::getText(
            this, tr("Add Environment Variable"),
            tr("Enter KEY=VALUE:"), QLineEdit::Normal, {}, &ok);
        if (ok && !kv.isEmpty())
            m_envList->addItem(kv);
    });
    connect(m_removeEnvBtn, &QPushButton::clicked, this, [this]{
        qDeleteAll(m_envList->selectedItems());
    });
    connect(m_deployBtn, &QPushButton::clicked, this, &ServiceDeployDialog::onDeployClicked);

    auto *envRow = new QHBoxLayout;
    envRow->addWidget(m_addEnvBtn);
    envRow->addWidget(m_removeEnvBtn);
    envRow->addStretch();

    auto *form = new QFormLayout;
    form->addRow(tr("Git Repo URL:"),   m_repoEdit);
    form->addRow(tr("Branch:"),         m_branchEdit);
    form->addRow(tr("Remote Path:"),    m_pathEdit);
    form->addRow(tr("Deploy Script:"),  m_scriptEdit);
    form->addRow(tr("Env Variables:"),  m_envList);
    form->addRow(QString(),             envRow);

    auto *bbox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *vbox = new QVBoxLayout(this);
    vbox->addLayout(form);
    vbox->addWidget(m_deployBtn);
    vbox->addWidget(new QLabel(tr("Output:"), this));
    vbox->addWidget(m_logEdit);
    vbox->addWidget(m_statusLabel);
    vbox->addWidget(bbox);
}

ServiceManager::DeployConfig ServiceDeployDialog::deployConfig() const
{
    ServiceManager::DeployConfig cfg;
    cfg.repoUrl      = m_repoEdit->text().trimmed();
    cfg.repoBranch   = m_branchEdit->text().trimmed();
    cfg.remotePath   = m_pathEdit->text().trimmed();
    cfg.deployScript = m_scriptEdit->text().trimmed();
    for (int i = 0; i < m_envList->count(); ++i)
        cfg.envVars << m_envList->item(i)->text();
    return cfg;
}

void ServiceDeployDialog::onDeployClicked()
{
    m_deployBtn->setEnabled(false);
    m_logEdit->clear();
    m_statusLabel->clear();

    const ServiceManager::DeployConfig cfg = deployConfig();
    if (cfg.repoUrl.isEmpty() || cfg.remotePath.isEmpty()) {
        QMessageBox::warning(this, tr("Input Required"),
            tr("Please provide the Git repository URL and remote path."));
        m_deployBtn->setEnabled(true);
        return;
    }

    ServiceManager mgr(this);
    connect(&mgr, &ServiceManager::progressLine, this, [this](const QString &line){
        m_logEdit->append(line);
    });

    const QString err = mgr.deployService(m_server, cfg);
    m_deployBtn->setEnabled(true);
    if (err.isEmpty()) {
        m_statusLabel->setText(tr("✓ Deployment completed successfully."));
    } else {
        m_statusLabel->setText(tr("✗ %1").arg(err));
    }
}

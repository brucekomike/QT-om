#include "cronjobeditdialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>

CronJobEditDialog::CronJobEditDialog(QWidget *parent) : QDialog(parent) { setupUi(); }
CronJobEditDialog::CronJobEditDialog(const CronJob &job, QWidget *parent)
    : QDialog(parent) { setupUi(); populateFrom(job); }

void CronJobEditDialog::setupUi()
{
    setWindowTitle(tr("Edit Cron Job"));
    setMinimumWidth(420);

    m_presetCombo = new QComboBox(this);
    m_presetCombo->addItem(tr("Custom"),          "");
    m_presetCombo->addItem(tr("Every minute"),    "* * * * *");
    m_presetCombo->addItem(tr("Every hour"),      "0 * * * *");
    m_presetCombo->addItem(tr("Daily at 2am"),    "0 2 * * *");
    m_presetCombo->addItem(tr("Weekly (Mon 3am)"), "0 3 * * 1");
    m_presetCombo->addItem(tr("Monthly (1st 4am)"), "0 4 1 * *");

    m_scheduleEdit = new QLineEdit(this);
    m_scheduleEdit->setPlaceholderText("* * * * *");

    m_commandEdit  = new QLineEdit(this);
    m_commandEdit->setPlaceholderText("/path/to/script.sh");

    m_commentEdit  = new QLineEdit(this);
    m_commentEdit->setPlaceholderText(tr("Unique identifier (used as ID)"));

    connect(m_presetCombo, &QComboBox::currentIndexChanged, this, [this](int idx){
        const QString val = m_presetCombo->itemData(idx).toString();
        if (!val.isEmpty()) m_scheduleEdit->setText(val);
    });

    auto *form = new QFormLayout;
    form->addRow(tr("Preset:"),    m_presetCombo);
    form->addRow(tr("Schedule:"),  m_scheduleEdit);
    form->addRow(tr("Command:"),   m_commandEdit);
    form->addRow(tr("Comment/ID:"), m_commentEdit);
    form->addRow(new QLabel(tr("<small>Schedule format: min hour day month weekday</small>"), this));

    auto *bbox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *vbox = new QVBoxLayout(this);
    vbox->addLayout(form);
    vbox->addWidget(bbox);
}

void CronJobEditDialog::populateFrom(const CronJob &job)
{
    m_scheduleEdit->setText(job.schedule);
    m_commandEdit->setText(job.command);
    m_commentEdit->setText(job.comment);
}

CronJob CronJobEditDialog::cronJob() const
{
    CronJob job;
    job.schedule = m_scheduleEdit->text().trimmed();
    job.command  = m_commandEdit->text().trimmed();
    job.comment  = m_commentEdit->text().trimmed();
    return job;
}

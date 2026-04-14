#pragma once
#include "managers/cronmanager.h"
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>

class CronJobEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CronJobEditDialog(QWidget *parent = nullptr);
    explicit CronJobEditDialog(const CronJob &job, QWidget *parent = nullptr);

    CronJob cronJob() const;

private:
    void setupUi();
    void populateFrom(const CronJob &job);

    QComboBox  *m_presetCombo;
    QLineEdit  *m_scheduleEdit;
    QLineEdit  *m_commandEdit;
    QLineEdit  *m_commentEdit;
};

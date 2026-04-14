#pragma once
#include "managers/configmanager.h"
#include "models/server.h"
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QList>

class ConfigMigrationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConfigMigrationDialog(const QList<Server> &servers, QWidget *parent = nullptr);

    QList<Server> importedServers() const { return m_importedServers; }

private slots:
    void onExportClicked();
    void onImportClicked();

private:
    void setupUi();

    QList<Server>  m_servers;
    QList<Server>  m_importedServers;
    QLineEdit     *m_passphraseEdit;
    QLabel        *m_statusLabel;
};

#pragma once
#include "managers/servermanager.h"
#include <QWidget>
#include <QListView>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>

class ServersPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ServersPanel(ServerManager *mgr, QWidget *parent = nullptr);

signals:
    void serverSelected(const Server &server);

private slots:
    void onAddServer();
    void onEditServer();
    void onRemoveServer();
    void onSelectionChanged();
    void onSetupKeyAuth();

private:
    void refreshDetails(const Server &server);

    ServerManager *m_mgr;
    QListView     *m_listView;
    QPushButton   *m_addBtn;
    QPushButton   *m_editBtn;
    QPushButton   *m_removeBtn;
    QPushButton   *m_keyAuthBtn;
    QLabel        *m_detailLabel;
    QTextEdit     *m_logEdit;
};

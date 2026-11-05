#ifndef MUDCONFIGDIALOG_H
#define MUDCONFIGDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include "powwow/powwow.h"

class MudConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit MudConfigDialog(powwow_session_t *session, QWidget *parent = nullptr);
    void refreshData();

private slots:
    void onAddAlias();
    void onDeleteAlias();
    void onAddAction();
    void onDeleteAction();
    void onAddVariable();
    void onDeleteVariable();

private:
    powwow_session_t *m_session;
    QTabWidget *m_tabs;
    QTableWidget *m_aliasTable;
    QTableWidget *m_actionTable;
    QTableWidget *m_varTable;
    QTableWidget *m_bindTable;
    QTableWidget *m_historyTable;
};

#endif // MUDCONFIGDIALOG_H

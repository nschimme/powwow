#include "mudconfigdialog.h"
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>

MudConfigDialog::MudConfigDialog(powwow_session_t *session, QWidget *parent)
    : QDialog(parent), m_session(session)
{
    setWindowTitle(tr("Powwow Struct Inspector"));
    resize(700, 450);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    mainLayout->addWidget(m_tabs);

    // Aliases Tab
    QWidget *aliasPage = new QWidget(this);
    QVBoxLayout *aliasLayout = new QVBoxLayout(aliasPage);
    m_aliasTable = new QTableWidget(0, 4, aliasPage);
    m_aliasTable->setHorizontalHeaderLabels({tr("Name"), tr("Substitution"), tr("Group"), tr("Enabled")});
    m_aliasTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    aliasLayout->addWidget(m_aliasTable);

    QHBoxLayout *aliasBtnLayout = new QHBoxLayout();
    QPushButton *addAliasBtn = new QPushButton(tr("Add Alias"), aliasPage);
    QPushButton *delAliasBtn = new QPushButton(tr("Delete Alias"), aliasPage);
    aliasBtnLayout->addWidget(addAliasBtn);
    aliasBtnLayout->addWidget(delAliasBtn);
    aliasLayout->addLayout(aliasBtnLayout);
    m_tabs->addTab(aliasPage, tr("Aliases"));

    connect(addAliasBtn, &QPushButton::clicked, this, &MudConfigDialog::onAddAlias);
    connect(delAliasBtn, &QPushButton::clicked, this, &MudConfigDialog::onDeleteAlias);

    // Actions / Triggers Tab
    QWidget *actionPage = new QWidget(this);
    QVBoxLayout *actionLayout = new QVBoxLayout(actionPage);
    m_actionTable = new QTableWidget(0, 4, actionPage);
    m_actionTable->setHorizontalHeaderLabels({tr("Pattern"), tr("Command"), tr("Group"), tr("Enabled")});
    m_actionTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    actionLayout->addWidget(m_actionTable);

    QHBoxLayout *actionBtnLayout = new QHBoxLayout();
    QPushButton *addActionBtn = new QPushButton(tr("Add Action"), actionPage);
    QPushButton *delActionBtn = new QPushButton(tr("Delete Action"), actionPage);
    actionBtnLayout->addWidget(addActionBtn);
    actionBtnLayout->addWidget(delActionBtn);
    actionLayout->addLayout(actionBtnLayout);
    m_tabs->addTab(actionPage, tr("Actions / Triggers"));

    connect(addActionBtn, &QPushButton::clicked, this, &MudConfigDialog::onAddAction);
    connect(delActionBtn, &QPushButton::clicked, this, &MudConfigDialog::onDeleteAction);

    // Variables Tab
    QWidget *varPage = new QWidget(this);
    QVBoxLayout *varLayout = new QVBoxLayout(varPage);
    m_varTable = new QTableWidget(0, 2, varPage);
    m_varTable->setHorizontalHeaderLabels({tr("Variable Name"), tr("Value")});
    m_varTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    varLayout->addWidget(m_varTable);

    QHBoxLayout *varBtnLayout = new QHBoxLayout();
    QPushButton *addVarBtn = new QPushButton(tr("Set Variable"), varPage);
    QPushButton *delVarBtn = new QPushButton(tr("Delete Variable"), varPage);
    varBtnLayout->addWidget(addVarBtn);
    varBtnLayout->addWidget(delVarBtn);
    varLayout->addLayout(varBtnLayout);
    m_tabs->addTab(varPage, tr("Variables"));

    connect(addVarBtn, &QPushButton::clicked, this, &MudConfigDialog::onAddVariable);
    connect(delVarBtn, &QPushButton::clicked, this, &MudConfigDialog::onDeleteVariable);

    // Key Binds Tab
    QWidget *bindPage = new QWidget(this);
    QVBoxLayout *bindLayout = new QVBoxLayout(bindPage);
    m_bindTable = new QTableWidget(0, 3, bindPage);
    m_bindTable->setHorizontalHeaderLabels({tr("Key Name"), tr("Sequence"), tr("Command")});
    m_bindTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    bindLayout->addWidget(m_bindTable);
    m_tabs->addTab(bindPage, tr("Key Binds"));

    // History Tab
    QWidget *histPage = new QWidget(this);
    QVBoxLayout *histLayout = new QVBoxLayout(histPage);
    m_historyTable = new QTableWidget(0, 2, histPage);
    m_historyTable->setHorizontalHeaderLabels({tr("#"), tr("Line")});
    m_historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    histLayout->addWidget(m_historyTable);
    m_tabs->addTab(histPage, tr("History"));

    // Close button
    QPushButton *closeBtn = new QPushButton(tr("Close"), this);
    mainLayout->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    refreshData();
}

void MudConfigDialog::refreshData()
{
    if (!m_session) return;

    // Refresh Aliases
    m_aliasTable->setRowCount(0);
    powwow_list_aliases(m_session, [](const powwow_alias_info_t *info, void *userData) {
        auto *table = static_cast<QTableWidget *>(userData);
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(info->name ? QString::fromUtf8(info->name) : QString()));
        table->setItem(row, 1, new QTableWidgetItem(info->subst ? QString::fromUtf8(info->subst) : QString()));
        table->setItem(row, 2, new QTableWidgetItem(info->group ? QString::fromUtf8(info->group) : QString()));
        table->setItem(row, 3, new QTableWidgetItem(info->enabled ? QString("Yes") : QString("No")));
    }, m_aliasTable);

    // Refresh Actions
    m_actionTable->setRowCount(0);
    powwow_list_actions(m_session, [](const powwow_action_info_t *info, void *userData) {
        auto *table = static_cast<QTableWidget *>(userData);
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(info->pattern ? QString::fromUtf8(info->pattern) : QString()));
        table->setItem(row, 1, new QTableWidgetItem(info->command ? QString::fromUtf8(info->command) : QString()));
        table->setItem(row, 2, new QTableWidgetItem(info->group ? QString::fromUtf8(info->group) : QString()));
        table->setItem(row, 3, new QTableWidgetItem(info->enabled ? QString("Yes") : QString("No")));
    }, m_actionTable);

    // Refresh Variables
    m_varTable->setRowCount(0);
    powwow_list_variables(m_session, [](const powwow_var_info_t *info, void *userData) {
        auto *table = static_cast<QTableWidget *>(userData);
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(info->name ? QString::fromUtf8(info->name) : QString()));
        table->setItem(row, 1, new QTableWidgetItem(info->value ? QString::fromUtf8(info->value) : QString()));
    }, m_varTable);

    // Refresh Key Binds
    m_bindTable->setRowCount(0);
    powwow_list_binds(m_session, [](const powwow_bind_info_t *info, void *userData) {
        auto *table = static_cast<QTableWidget *>(userData);
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(info->name ? QString::fromUtf8(info->name) : QString()));
        table->setItem(row, 1, new QTableWidgetItem(info->sequence ? QString::fromUtf8(info->sequence) : QString()));
        table->setItem(row, 2, new QTableWidgetItem(info->command ? QString::fromUtf8(info->command) : QString()));
    }, m_bindTable);

    // Refresh History
    m_historyTable->setRowCount(0);
    powwow_list_history(m_session, [](const powwow_history_info_t *info, void *userData) {
        auto *table = static_cast<QTableWidget *>(userData);
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(QString::number(info->index)));
        table->setItem(row, 1, new QTableWidgetItem(info->line ? QString::fromUtf8(info->line) : QString()));
    }, m_historyTable);
}

void MudConfigDialog::onAddAlias()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Alias"), tr("Alias Name:"), QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString subst = QInputDialog::getText(this, tr("Add Alias"), tr("Substitution:"), QLineEdit::Normal, "", &ok);
    if (!ok) return;

    powwow_set_alias(m_session, name.trimmed().toUtf8().constData(), subst.toUtf8().constData(), nullptr);
    refreshData();
}

void MudConfigDialog::onDeleteAlias()
{
    int row = m_aliasTable->currentRow();
    if (row < 0) return;
    QString name = m_aliasTable->item(row, 0)->text();
    powwow_delete_alias(m_session, name.toUtf8().constData());
    refreshData();
}

void MudConfigDialog::onAddAction()
{
    bool ok;
    QString pattern = QInputDialog::getText(this, tr("Add Action"), tr("Pattern:"), QLineEdit::Normal, "", &ok);
    if (!ok || pattern.trimmed().isEmpty()) return;

    QString cmd = QInputDialog::getText(this, tr("Add Action"), tr("Command:"), QLineEdit::Normal, "", &ok);
    if (!ok) return;

    powwow_set_action(m_session, pattern.toUtf8().constData(), cmd.toUtf8().constData(), nullptr);
    refreshData();
}

void MudConfigDialog::onDeleteAction()
{
    int row = m_actionTable->currentRow();
    if (row < 0) return;
    QString pattern = m_actionTable->item(row, 0)->text();
    powwow_delete_action(m_session, pattern.toUtf8().constData());
    refreshData();
}

void MudConfigDialog::onAddVariable()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Set Variable"), tr("Variable Name:"), QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString val = QInputDialog::getText(this, tr("Set Variable"), tr("Value:"), QLineEdit::Normal, "", &ok);
    if (!ok) return;

    powwow_set_variable(m_session, name.trimmed().toUtf8().constData(), val.toUtf8().constData());
    refreshData();
}

void MudConfigDialog::onDeleteVariable()
{
    int row = m_varTable->currentRow();
    if (row < 0) return;
    QString name = m_varTable->item(row, 0)->text();
    powwow_delete_variable(m_session, name.toUtf8().constData());
    refreshData();
}

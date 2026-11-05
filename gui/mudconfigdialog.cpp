#include "mudconfigdialog.h"
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>

MudConfigDialog::MudConfigDialog(powwow_session_t *session, QWidget *parent)
    : QDialog(parent), m_session(session)
{
    setWindowTitle(tr("MUD Configuration & Inspector (libpowwow)"));
    resize(750, 480);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    mainLayout->addWidget(m_tabs);

    // 1. Aliases Tab
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

    // 2. Actions / Triggers Tab
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

    // 3. Variables Tab
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

    // 4. Key Binds Tab
    QWidget *bindPage = new QWidget(this);
    QVBoxLayout *bindLayout = new QVBoxLayout(bindPage);
    m_bindTable = new QTableWidget(0, 3, bindPage);
    m_bindTable->setHorizontalHeaderLabels({tr("Key Name"), tr("Sequence"), tr("Command")});
    m_bindTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    bindLayout->addWidget(m_bindTable);

    QHBoxLayout *bindBtnLayout = new QHBoxLayout();
    QPushButton *addBindBtn = new QPushButton(tr("Add Key Bind"), bindPage);
    QPushButton *delBindBtn = new QPushButton(tr("Delete Key Bind"), bindPage);
    bindBtnLayout->addWidget(addBindBtn);
    bindBtnLayout->addWidget(delBindBtn);
    bindLayout->addLayout(bindBtnLayout);
    m_tabs->addTab(bindPage, tr("Key Binds"));

    connect(addBindBtn, &QPushButton::clicked, this, &MudConfigDialog::onAddBind);
    connect(delBindBtn, &QPushButton::clicked, this, &MudConfigDialog::onDeleteBind);

    // 5. Marks / Highlights Tab
    QWidget *markPage = new QWidget(this);
    QVBoxLayout *markLayout = new QVBoxLayout(markPage);
    m_markTable = new QTableWidget(0, 3, markPage);
    m_markTable->setHorizontalHeaderLabels({tr("Pattern"), tr("Attr Code"), tr("Start of Line")});
    m_markTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    markLayout->addWidget(m_markTable);

    QHBoxLayout *markBtnLayout = new QHBoxLayout();
    QPushButton *addMarkBtn = new QPushButton(tr("Add Mark"), markPage);
    QPushButton *delMarkBtn = new QPushButton(tr("Delete Mark"), markPage);
    markBtnLayout->addWidget(addMarkBtn);
    markBtnLayout->addWidget(delMarkBtn);
    markLayout->addLayout(markBtnLayout);
    m_tabs->addTab(markPage, tr("Marks / Highlights"));

    connect(addMarkBtn, &QPushButton::clicked, this, &MudConfigDialog::onAddMark);
    connect(delMarkBtn, &QPushButton::clicked, this, &MudConfigDialog::onDeleteMark);

    // 6. Delays / Timers Tab
    QWidget *delayPage = new QWidget(this);
    QVBoxLayout *delayLayout = new QVBoxLayout(delayPage);
    m_delayTable = new QTableWidget(0, 4, delayPage);
    m_delayTable->setHorizontalHeaderLabels({tr("Name"), tr("Command"), tr("Delay (ms)"), tr("Active")});
    m_delayTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    delayLayout->addWidget(m_delayTable);

    QHBoxLayout *delayBtnLayout = new QHBoxLayout();
    QPushButton *delDelayBtn = new QPushButton(tr("Delete Delay / Timer"), delayPage);
    delayBtnLayout->addWidget(delDelayBtn);
    delayLayout->addLayout(delayBtnLayout);
    m_tabs->addTab(delayPage, tr("Delays / Timers"));

    connect(delDelayBtn, &QPushButton::clicked, this, &MudConfigDialog::onDeleteDelay);

    // 7. History Tab
    QWidget *histPage = new QWidget(this);
    QVBoxLayout *histLayout = new QVBoxLayout(histPage);
    m_historyTable = new QTableWidget(0, 2, histPage);
    m_historyTable->setHorizontalHeaderLabels({tr("#"), tr("Line")});
    m_historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    histLayout->addWidget(m_historyTable);

    QHBoxLayout *histBtnLayout = new QHBoxLayout();
    QPushButton *addHistBtn = new QPushButton(tr("Add to History"), histPage);
    histBtnLayout->addWidget(addHistBtn);
    histLayout->addLayout(histBtnLayout);
    m_tabs->addTab(histPage, tr("History"));

    connect(addHistBtn, &QPushButton::clicked, this, &MudConfigDialog::onAddHistory);

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

    // Refresh Marks
    m_markTable->setRowCount(0);
    powwow_list_marks(m_session, [](const powwow_mark_info_t *info, void *userData) {
        auto *table = static_cast<QTableWidget *>(userData);
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(info->pattern ? QString::fromUtf8(info->pattern) : QString()));
        table->setItem(row, 1, new QTableWidgetItem(QString::number(info->attrcode)));
        table->setItem(row, 2, new QTableWidgetItem(info->at_beg ? QString("Yes") : QString("No")));
    }, m_markTable);

    // Refresh Delays
    m_delayTable->setRowCount(0);
    powwow_list_delays(m_session, [](const powwow_delay_info_t *info, void *userData) {
        auto *table = static_cast<QTableWidget *>(userData);
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(info->name ? QString::fromUtf8(info->name) : QString()));
        table->setItem(row, 1, new QTableWidgetItem(info->command ? QString::fromUtf8(info->command) : QString()));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(info->delay_ms)));
        table->setItem(row, 3, new QTableWidgetItem(info->is_active ? QString("Yes") : QString("No")));
    }, m_delayTable);

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

void MudConfigDialog::onAddBind()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Key Bind"), tr("Key Name:"), QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString seq = QInputDialog::getText(this, tr("Add Key Bind"), tr("Escape Sequence:"), QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QString cmd = QInputDialog::getText(this, tr("Add Key Bind"), tr("Command:"), QLineEdit::Normal, "", &ok);
    if (!ok) return;

    powwow_set_bind(m_session, name.trimmed().toUtf8().constData(), seq.toUtf8().constData(), cmd.toUtf8().constData());
    refreshData();
}

void MudConfigDialog::onDeleteBind()
{
    int row = m_bindTable->currentRow();
    if (row < 0) return;
    QString name = m_bindTable->item(row, 0)->text();
    powwow_delete_bind(m_session, name.toUtf8().constData());
    refreshData();
}

void MudConfigDialog::onAddMark()
{
    bool ok;
    QString pattern = QInputDialog::getText(this, tr("Add Mark"), tr("Pattern:"), QLineEdit::Normal, "", &ok);
    if (!ok || pattern.trimmed().isEmpty()) return;

    int attr = QInputDialog::getInt(this, tr("Add Mark"), tr("Attribute Code (e.g. 1=bold):"), 1, 0, 255, 1, &ok);
    if (!ok) return;

    powwow_set_mark(m_session, pattern.trimmed().toUtf8().constData(), attr, false);
    refreshData();
}

void MudConfigDialog::onDeleteMark()
{
    int row = m_markTable->currentRow();
    if (row < 0) return;
    QString pattern = m_markTable->item(row, 0)->text();
    powwow_delete_mark(m_session, pattern.toUtf8().constData(), false);
    refreshData();
}

void MudConfigDialog::onDeleteDelay()
{
    int row = m_delayTable->currentRow();
    if (row < 0) return;
    QString name = m_delayTable->item(row, 0)->text();
    powwow_delete_delay(m_session, name.toUtf8().constData());
    refreshData();
}

void MudConfigDialog::onAddHistory()
{
    bool ok;
    QString line = QInputDialog::getText(this, tr("Add History"), tr("Command Line:"), QLineEdit::Normal, "", &ok);
    if (!ok || line.trimmed().isEmpty()) return;

    powwow_add_history(m_session, line.trimmed().toUtf8().constData());
    refreshData();
}

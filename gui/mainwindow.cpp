#include "mainwindow.h"
#include "terminalwidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Powwow GUI Configuration & Launcher"));
    resize(760, 520);

    setupUi();
    refreshSessionList();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // Configuration page
    m_configPage = new QWidget(this);
    QHBoxLayout *mainConfigLayout = new QHBoxLayout(m_configPage);

    // Left Panel: Saved Sessions (PuTTY style)
    QGroupBox *sessionGroupBox = new QGroupBox(tr("Saved Sessions"), m_configPage);
    QVBoxLayout *sessionLayout = new QVBoxLayout(sessionGroupBox);

    m_sessionListWidget = new QListWidget(sessionGroupBox);
    sessionLayout->addWidget(m_sessionListWidget);

    QHBoxLayout *sessionButtonsLayout = new QHBoxLayout();
    m_saveButton = new QPushButton(tr("Save"), sessionGroupBox);
    m_deleteButton = new QPushButton(tr("Delete"), sessionGroupBox);
    sessionButtonsLayout->addWidget(m_saveButton);
    sessionButtonsLayout->addWidget(m_deleteButton);
    sessionLayout->addLayout(sessionButtonsLayout);

    mainConfigLayout->addWidget(sessionGroupBox, 1);

    // Right Panel: Basic Options
    QGroupBox *optionsGroupBox = new QGroupBox(tr("Session Settings"), m_configPage);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroupBox);

    QFormLayout *formLayout = new QFormLayout();

    m_sessionNameEdit = new QLineEdit(optionsGroupBox);
    m_sessionNameEdit->setPlaceholderText(tr("New Session Name"));
    formLayout->addRow(tr("Session Name:"), m_sessionNameEdit);

    m_hostEdit = new QLineEdit(optionsGroupBox);
    m_hostEdit->setPlaceholderText(tr("e.g. mume.org"));
    formLayout->addRow(tr("Host Name (or IP):"), m_hostEdit);

    m_portSpinBox = new QSpinBox(optionsGroupBox);
    m_portSpinBox->setRange(1, 65535);
    m_portSpinBox->setValue(23);
    formLayout->addRow(tr("Port:"), m_portSpinBox);

    // Profile (.pow file) path picker
    QHBoxLayout *powFileLayout = new QHBoxLayout();
    m_powFileEdit = new QLineEdit(optionsGroupBox);
    m_powFileEdit->setPlaceholderText(tr("Path to .pow profile file (optional)"));
    m_browsePowButton = new QPushButton(tr("Browse..."), optionsGroupBox);
    powFileLayout->addWidget(m_powFileEdit);
    powFileLayout->addWidget(m_browsePowButton);
    formLayout->addRow(tr("Profile (.pow):"), powFileLayout);

    // Editor (POWWOWEDITOR) picker
    QHBoxLayout *editorLayout = new QHBoxLayout();
    m_editorEdit = new QLineEdit(optionsGroupBox);
    m_editorEdit->setPlaceholderText(tr("Editor program (e.g. vi, emacs, gedit)"));
    m_browseEditorButton = new QPushButton(tr("Browse..."), optionsGroupBox);
    editorLayout->addWidget(m_editorEdit);
    editorLayout->addWidget(m_browseEditorButton);
    formLayout->addRow(tr("Editor (POWWOWEDITOR):"), editorLayout);

    // Pager (POWWOWPAGER) picker
    QHBoxLayout *pagerLayout = new QHBoxLayout();
    m_pagerEdit = new QLineEdit(optionsGroupBox);
    m_pagerEdit->setPlaceholderText(tr("Pager/Viewer program (e.g. less, more)"));
    m_browsePagerButton = new QPushButton(tr("Browse..."), optionsGroupBox);
    pagerLayout->addWidget(m_pagerEdit);
    pagerLayout->addWidget(m_browsePagerButton);
    formLayout->addRow(tr("Pager (POWWOWPAGER):"), pagerLayout);

    m_tlsCheckBox = new QCheckBox(tr("Enable TLS Encryption"), optionsGroupBox);
    formLayout->addRow(tr("Security:"), m_tlsCheckBox);

    m_utf8CheckBox = new QCheckBox(tr("Enable UTF-8 Encoding"), optionsGroupBox);
    m_utf8CheckBox->setChecked(true);
    formLayout->addRow(tr("Encoding:"), m_utf8CheckBox);

    optionsLayout->addLayout(formLayout);
    optionsLayout->addStretch();

    // Connect / Open button (PuTTY style Open button at bottom right)
    QHBoxLayout *connectLayout = new QHBoxLayout();
    connectLayout->addStretch();
    m_connectButton = new QPushButton(tr("Connect"), optionsGroupBox);
    m_connectButton->setDefault(true);
    m_connectButton->setMinimumHeight(32);
    m_connectButton->setMinimumWidth(100);
    connectLayout->addWidget(m_connectButton);

    optionsLayout->addLayout(connectLayout);

    mainConfigLayout->addWidget(optionsGroupBox, 2);

    m_stackedWidget->addWidget(m_configPage);

    // Terminal page (tabbed interface for active sessions)
    m_terminalTabWidget = new QTabWidget(this);
    m_terminalTabWidget->setTabsClosable(true);
    m_stackedWidget->addWidget(m_terminalTabWidget);

    // Connect signals
    connect(m_sessionListWidget, &QListWidget::currentRowChanged, this, &MainWindow::onSessionSelected);
    connect(m_browsePowButton, &QPushButton::clicked, this, &MainWindow::onBrowsePowFile);
    connect(m_browseEditorButton, &QPushButton::clicked, this, &MainWindow::onBrowseEditor);
    connect(m_browsePagerButton, &QPushButton::clicked, this, &MainWindow::onBrowsePager);
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::onSaveSession);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteSession);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectSession);
    connect(m_terminalTabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onCloseTerminalTab);
}

void MainWindow::refreshSessionList()
{
    m_sessionListWidget->clear();
    m_sessionListWidget->addItem(tr("Default Settings"));
    const QStringList sessions = m_profileManager.sessionNames();
    for (const QString &s : sessions) {
        m_sessionListWidget->addItem(s);
    }
    m_sessionListWidget->setCurrentRow(0);
}

void MainWindow::onSessionSelected(int index)
{
    if (index < 0) return;
    QString sessionName = m_sessionListWidget->item(index)->text();
    if (sessionName == tr("Default Settings")) {
        m_sessionNameEdit->clear();
        m_hostEdit->clear();
        m_portSpinBox->setValue(23);
        m_powFileEdit->clear();
        m_editorEdit->clear();
        m_pagerEdit->clear();
        m_tlsCheckBox->setChecked(false);
        m_utf8CheckBox->setChecked(true);
    } else {
        SessionProfile profile = m_profileManager.loadSession(sessionName);
        m_sessionNameEdit->setText(profile.name);
        m_hostEdit->setText(profile.host);
        m_portSpinBox->setValue(profile.port);
        m_powFileEdit->setText(profile.powFilePath);
        m_editorEdit->setText(profile.editor);
        m_pagerEdit->setText(profile.pager);
        m_tlsCheckBox->setChecked(profile.useTls);
        m_utf8CheckBox->setChecked(profile.useUtf8);
    }
}

void MainWindow::onBrowsePowFile()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("Select Powwow Profile File"),
        QString(),
        tr("Powwow Files (*.pow *.doc *.demo);;All Files (*)"));
    if (!filePath.isEmpty()) {
        m_powFileEdit->setText(filePath);
    }
}

void MainWindow::onBrowseEditor()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("Select Editor Executable"),
        QString(),
        tr("Executables (*);;All Files (*)"));
    if (!filePath.isEmpty()) {
        m_editorEdit->setText(filePath);
    }
}

void MainWindow::onBrowsePager()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("Select Pager Executable"),
        QString(),
        tr("Executables (*);;All Files (*)"));
    if (!filePath.isEmpty()) {
        m_pagerEdit->setText(filePath);
    }
}

SessionProfile MainWindow::currentProfileFromUi() const
{
    SessionProfile profile;
    profile.name = m_sessionNameEdit->text().trimmed();
    profile.host = m_hostEdit->text().trimmed();
    profile.port = m_portSpinBox->value();
    profile.powFilePath = m_powFileEdit->text().trimmed();
    profile.editor = m_editorEdit->text().trimmed();
    profile.pager = m_pagerEdit->text().trimmed();
    profile.useTls = m_tlsCheckBox->isChecked();
    profile.useUtf8 = m_utf8CheckBox->isChecked();
    return profile;
}

void MainWindow::onSaveSession()
{
    SessionProfile profile = currentProfileFromUi();
    if (profile.name.isEmpty()) {
        QMessageBox::warning(this, tr("Save Error"), tr("Please enter a Session Name before saving."));
        return;
    }
    m_profileManager.saveSession(profile);
    refreshSessionList();
    // Re-select saved session
    for (int i = 0; i < m_sessionListWidget->count(); ++i) {
        if (m_sessionListWidget->item(i)->text() == profile.name) {
            m_sessionListWidget->setCurrentRow(i);
            break;
        }
    }
}

void MainWindow::onDeleteSession()
{
    int row = m_sessionListWidget->currentRow();
    if (row <= 0) return; // Cannot delete default
    QString name = m_sessionListWidget->item(row)->text();
    m_profileManager.deleteSession(name);
    refreshSessionList();
}

void MainWindow::onConnectSession()
{
    SessionProfile profile = currentProfileFromUi();

    TerminalWidget *termWidget = new TerminalWidget(profile, this);
    QString tabTitle = profile.name.isEmpty() ?
        (!profile.host.isEmpty() ? profile.host : tr("Session")) : profile.name;

    int tabIndex = m_terminalTabWidget->addTab(termWidget, tabTitle);
    m_terminalTabWidget->setCurrentIndex(tabIndex);
    m_stackedWidget->setCurrentWidget(m_terminalTabWidget);

    connect(termWidget, &TerminalWidget::sessionClosed, [this, termWidget]() {
        int idx = m_terminalTabWidget->indexOf(termWidget);
        if (idx != -1) {
            onCloseTerminalTab(idx);
        }
    });

    termWidget->startSession();
}

void MainWindow::onCloseTerminalTab(int index)
{
    QWidget *w = m_terminalTabWidget->widget(index);
    m_terminalTabWidget->removeTab(index);
    delete w;

    if (m_terminalTabWidget->count() == 0) {
        m_stackedWidget->setCurrentWidget(m_configPage);
    }
}

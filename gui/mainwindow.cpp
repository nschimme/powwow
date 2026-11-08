#include "mainwindow.h"
#include "terminalwidget.h"
#include "mudconfigdialog.h"

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
    setWindowTitle(tr("PowTTY - Modern MUD Client"));
    resize(980, 640);

    setupUi();
    refreshSessionList();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_mainSplitter);

    // 1. Persistent Left Sidebar
    m_sidebarWidget = new QWidget(m_mainSplitter);
    m_sidebarWidget->setMinimumWidth(200);
    m_sidebarWidget->setMaximumWidth(280);
    QVBoxLayout *sidebarLayout = new QVBoxLayout(m_sidebarWidget);
    sidebarLayout->setContentsMargins(8, 8, 8, 8);
    sidebarLayout->setSpacing(6);

    QLabel *sidebarHeader = new QLabel(tr("⚡ MUD WORLDS"), m_sidebarWidget);
    sidebarHeader->setStyleSheet("font-weight: bold; color: #4ec9b0; font-size: 11px; letter-spacing: 1px; padding: 4px 2px;");
    sidebarLayout->addWidget(sidebarHeader);

    m_sessionListWidget = new QListWidget(m_sidebarWidget);
    sidebarLayout->addWidget(m_sessionListWidget, 1);

    QHBoxLayout *sideBtnLayout = new QHBoxLayout();
    m_newSessionButton = new QPushButton(tr("＋ New"), m_sidebarWidget);
    m_deleteButton = new QPushButton(tr("🗑 Delete"), m_sidebarWidget);
    m_newSessionButton->setStyleSheet("background-color: #252526;");
    m_deleteButton->setStyleSheet("background-color: #252526;");
    sideBtnLayout->addWidget(m_newSessionButton);
    sideBtnLayout->addWidget(m_deleteButton);
    sidebarLayout->addLayout(sideBtnLayout);

    m_mainSplitter->addWidget(m_sidebarWidget);

    // 2. Right Content Stack (Configuration Form or Active Terminal)
    m_contentStack = new QStackedWidget(m_mainSplitter);

    // Configuration Page (Card-style view)
    m_configPage = new QWidget(m_contentStack);
    QVBoxLayout *configPageLayout = new QVBoxLayout(m_configPage);
    configPageLayout->setContentsMargins(16, 16, 16, 16);

    QGroupBox *optionsGroupBox = new QGroupBox(tr("World Connection Settings"), m_configPage);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroupBox);
    optionsLayout->setContentsMargins(16, 18, 16, 16);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(10);

    m_sessionNameEdit = new QLineEdit(optionsGroupBox);
    m_sessionNameEdit->setPlaceholderText(tr("e.g. MUME, Achaea"));
    formLayout->addRow(tr("Profile Name:"), m_sessionNameEdit);

    QHBoxLayout *hostPortLayout = new QHBoxLayout();
    m_hostEdit = new QLineEdit(optionsGroupBox);
    m_hostEdit->setText("mume.org");
    m_hostEdit->setPlaceholderText(tr("Host Address (e.g. mume.org)"));

    m_portSpinBox = new QSpinBox(optionsGroupBox);
    m_portSpinBox->setRange(1, 65535);
    m_portSpinBox->setValue(4242);

    hostPortLayout->addWidget(m_hostEdit, 3);
    hostPortLayout->addWidget(new QLabel(tr("Port:"), optionsGroupBox));
    hostPortLayout->addWidget(m_portSpinBox, 1);
    formLayout->addRow(tr("Server Host & Port:"), hostPortLayout);

    // Profile (.pow file) path picker
    QHBoxLayout *powFileLayout = new QHBoxLayout();
    m_powFileEdit = new QLineEdit(optionsGroupBox);
    m_powFileEdit->setText(defaultPowFilePath());
    m_powFileEdit->setPlaceholderText(tr("Path to .pow profile file"));
    m_browsePowButton = new QPushButton(tr("Browse..."), optionsGroupBox);
    powFileLayout->addWidget(m_powFileEdit);
    powFileLayout->addWidget(m_browsePowButton);
    formLayout->addRow(tr("Profile File (.pow):"), powFileLayout);

    // Editor picker
    QHBoxLayout *editorLayout = new QHBoxLayout();
    m_editorEdit = new QLineEdit(optionsGroupBox);
    m_editorEdit->setText(defaultPlatformEditor());
    m_editorEdit->setPlaceholderText(tr("Default (%1)").arg(defaultPlatformEditor()));
    m_browseEditorButton = new QPushButton(tr("Browse..."), optionsGroupBox);
    editorLayout->addWidget(m_editorEdit);
    editorLayout->addWidget(m_browseEditorButton);
    formLayout->addRow(tr("External Editor:"), editorLayout);

    // Pager picker
    QHBoxLayout *pagerLayout = new QHBoxLayout();
    m_pagerEdit = new QLineEdit(optionsGroupBox);
    m_pagerEdit->setText(defaultPlatformPager());
    m_pagerEdit->setPlaceholderText(tr("Default (%1)").arg(defaultPlatformPager()));
    m_browsePagerButton = new QPushButton(tr("Browse..."), optionsGroupBox);
    pagerLayout->addWidget(m_pagerEdit);
    pagerLayout->addWidget(m_browsePagerButton);
    formLayout->addRow(tr("External Pager:"), pagerLayout);

    // Font selection
    QHBoxLayout *fontLayout = new QHBoxLayout();
    m_fontComboBox = new QFontComboBox(optionsGroupBox);
    m_fontComboBox->setFontFilters(QFontComboBox::MonospacedFonts);
    m_fontComboBox->setCurrentFont(QFont("PowTTY Mono"));

    m_fontSizeSpinBox = new QSpinBox(optionsGroupBox);
    m_fontSizeSpinBox->setRange(6, 72);
    m_fontSizeSpinBox->setValue(10);

    fontLayout->addWidget(m_fontComboBox, 3);
    fontLayout->addWidget(new QLabel(tr("Size:"), optionsGroupBox));
    fontLayout->addWidget(m_fontSizeSpinBox, 1);
    formLayout->addRow(tr("Terminal Font:"), fontLayout);

    m_scrollbackSpinBox = new QSpinBox(optionsGroupBox);
    m_scrollbackSpinBox->setRange(100, 100000);
    m_scrollbackSpinBox->setSingleStep(500);
    m_scrollbackSpinBox->setValue(5000);
    formLayout->addRow(tr("Scrollback Buffer:"), m_scrollbackSpinBox);

    QHBoxLayout *checksLayout = new QHBoxLayout();
    m_tlsCheckBox = new QCheckBox(tr("TLS Encryption (SSL)"), optionsGroupBox);
    m_tlsCheckBox->setChecked(false);
    m_utf8CheckBox = new QCheckBox(tr("UTF-8 Character Encoding"), optionsGroupBox);
    m_utf8CheckBox->setChecked(true);
    checksLayout->addWidget(m_tlsCheckBox);
    checksLayout->addWidget(m_utf8CheckBox);
    checksLayout->addStretch();
    formLayout->addRow(tr("Network Options:"), checksLayout);

    optionsLayout->addLayout(formLayout);
    optionsLayout->addStretch();

    // Bottom action row
    QHBoxLayout *actionsRow = new QHBoxLayout();
    m_saveButton = new QPushButton(tr("💾 Save World"), optionsGroupBox);
    m_saveButton->setStyleSheet("background-color: #333333;");
    actionsRow->addWidget(m_saveButton);

    m_inspectButton = new QPushButton(tr("⚡ MUD Configuration"), optionsGroupBox);
    m_inspectButton->setStyleSheet("background-color: #333333;");
    actionsRow->addWidget(m_inspectButton);

    actionsRow->addStretch();

    m_connectButton = new QPushButton(tr("🚀 Connect to World"), optionsGroupBox);
    m_connectButton->setDefault(true);
    m_connectButton->setMinimumHeight(36);
    m_connectButton->setMinimumWidth(150);
    m_connectButton->setStyleSheet("QPushButton { font-weight: bold; font-size: 14px; background-color: #0e639c; } QPushButton:hover { background-color: #1177bb; }");
    actionsRow->addWidget(m_connectButton);

    optionsLayout->addLayout(actionsRow);

    configPageLayout->addWidget(optionsGroupBox);
    m_contentStack->addWidget(m_configPage);

    m_mainSplitter->addWidget(m_contentStack);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);

    // Connect signals
    connect(m_sessionListWidget, &QListWidget::currentRowChanged, this, &MainWindow::onSessionSelected);
    connect(m_newSessionButton, &QPushButton::clicked, this, &MainWindow::onNewSession);
    connect(m_browsePowButton, &QPushButton::clicked, this, &MainWindow::onBrowsePowFile);
    connect(m_browseEditorButton, &QPushButton::clicked, this, &MainWindow::onBrowseEditor);
    connect(m_browsePagerButton, &QPushButton::clicked, this, &MainWindow::onBrowsePager);
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::onSaveSession);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteSession);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectSession);
    connect(m_inspectButton, &QPushButton::clicked, this, &MainWindow::onInspectStructs);
}

void MainWindow::onNewSession()
{
    m_sessionListWidget->setCurrentRow(0);
    m_sessionNameEdit->clear();
    m_hostEdit->setText("mume.org");
    m_portSpinBox->setValue(4242);
    m_sessionNameEdit->setFocus();
}

void MainWindow::onInspectStructs()
{
    powwow_session_t *sess = m_activeTerminalWidget ? m_activeTerminalWidget->powwowSession() : nullptr;
    bool created = false;
    if (!sess) {
        powwow_ui_ops_t ops = {};
        sess = powwow_session_create(&ops, nullptr);
        created = true;
    }
    MudConfigDialog dlg(sess, this);
    dlg.exec();
    if (created && sess) {
        powwow_session_destroy(sess);
    }
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
        m_hostEdit->setText("mume.org");
        m_portSpinBox->setValue(4242);
        m_powFileEdit->setText(defaultPowFilePath());
        m_editorEdit->setText(defaultPlatformEditor());
        m_pagerEdit->setText(defaultPlatformPager());
        m_scrollbackSpinBox->setValue(5000);
        m_tlsCheckBox->setChecked(false);
        m_utf8CheckBox->setChecked(true);
        m_fontComboBox->setCurrentFont(QFont("PowTTY Mono"));
        m_fontSizeSpinBox->setValue(10);
    } else {
        SessionProfile profile = m_profileManager.loadSession(sessionName);
        m_sessionNameEdit->setText(profile.name);
        m_hostEdit->setText(profile.host.isEmpty() ? "mume.org" : profile.host);
        m_portSpinBox->setValue(profile.port > 0 ? profile.port : 4242);
        m_powFileEdit->setText(profile.powFilePath.isEmpty() ? defaultPowFilePath() : profile.powFilePath);
        m_editorEdit->setText(profile.editor.isEmpty() ? defaultPlatformEditor() : profile.editor);
        m_pagerEdit->setText(profile.pager.isEmpty() ? defaultPlatformPager() : profile.pager);
        m_scrollbackSpinBox->setValue(profile.scrollbackLines > 0 ? profile.scrollbackLines : 5000);
        m_tlsCheckBox->setChecked(profile.useTls);
        m_utf8CheckBox->setChecked(profile.useUtf8);
        m_fontComboBox->setCurrentFont(QFont(profile.fontFamily.isEmpty() ? "PowTTY Mono" : profile.fontFamily));
        m_fontSizeSpinBox->setValue(profile.fontSize > 0 ? profile.fontSize : 10);
    }
}

void MainWindow::onBrowsePowFile()
{
    QString initialDir = profileDirectoryPath();
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("Select Powwow Profile File"),
        initialDir,
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
    profile.host = m_hostEdit->text().trimmed().isEmpty() ? "mume.org" : m_hostEdit->text().trimmed();
    profile.port = m_portSpinBox->value();
    profile.powFilePath = m_powFileEdit->text().trimmed().isEmpty() ? defaultPowFilePath() : m_powFileEdit->text().trimmed();
    profile.editor = m_editorEdit->text().trimmed().isEmpty() ? defaultPlatformEditor() : m_editorEdit->text().trimmed();
    profile.pager = m_pagerEdit->text().trimmed().isEmpty() ? defaultPlatformPager() : m_pagerEdit->text().trimmed();
    profile.scrollbackLines = m_scrollbackSpinBox->value();
    profile.useTls = m_tlsCheckBox->isChecked();
    profile.useUtf8 = m_utf8CheckBox->isChecked();
    profile.fontFamily = m_fontComboBox->currentFont().family();
    profile.fontSize = m_fontSizeSpinBox->value();
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
    if (m_activeTerminalWidget) {
        m_contentStack->removeWidget(m_activeTerminalWidget);
        delete m_activeTerminalWidget;
        m_activeTerminalWidget = nullptr;
    }

    SessionProfile profile = currentProfileFromUi();

    // Auto-hide the sidebar when connecting to give 100% focus to playing
    m_sidebarWidget->hide();

    m_activeTerminalWidget = new TerminalWidget(profile, this);
    m_contentStack->addWidget(m_activeTerminalWidget);
    m_contentStack->setCurrentWidget(m_activeTerminalWidget);

    connect(m_activeTerminalWidget, &TerminalWidget::sessionClosed, this, &MainWindow::onEndSession);

    m_activeTerminalWidget->startSession();
}

void MainWindow::onEndSession()
{
    m_sidebarWidget->show();
    if (m_activeTerminalWidget) {
        m_contentStack->setCurrentWidget(m_configPage);
        m_contentStack->removeWidget(m_activeTerminalWidget);
        m_activeTerminalWidget->deleteLater();
        m_activeTerminalWidget = nullptr;
    }
}

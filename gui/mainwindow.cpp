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
    setWindowTitle(tr("PowTTY"));
    resize(750, 520);

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
    m_sessionNameEdit->setPlaceholderText(tr("Session Name"));
    formLayout->addRow(tr("Session Name:"), m_sessionNameEdit);

    QHBoxLayout *hostPortLayout = new QHBoxLayout();
    m_hostEdit = new QLineEdit(optionsGroupBox);
    m_hostEdit->setText("mume.org");
    m_hostEdit->setPlaceholderText(tr("mume.org"));

    m_portSpinBox = new QSpinBox(optionsGroupBox);
    m_portSpinBox->setRange(1, 65535);
    m_portSpinBox->setValue(4242);

    hostPortLayout->addWidget(m_hostEdit, 3);
    hostPortLayout->addWidget(new QLabel(tr("Port:"), optionsGroupBox));
    hostPortLayout->addWidget(m_portSpinBox, 1);
    formLayout->addRow(tr("Host Name:"), hostPortLayout);

    // Profile (.pow file) path picker
    QHBoxLayout *powFileLayout = new QHBoxLayout();
    m_powFileEdit = new QLineEdit(optionsGroupBox);
    m_powFileEdit->setText(defaultPowFilePath());
    m_powFileEdit->setPlaceholderText(tr("Path to .pow profile file"));
    m_browsePowButton = new QPushButton(tr("Browse..."), optionsGroupBox);
    powFileLayout->addWidget(m_powFileEdit);
    powFileLayout->addWidget(m_browsePowButton);
    formLayout->addRow(tr("Profile (.pow):"), powFileLayout);

    // Editor (POWWOWEDITOR) picker with platform default placeholder
    QHBoxLayout *editorLayout = new QHBoxLayout();
    m_editorEdit = new QLineEdit(optionsGroupBox);
    m_editorEdit->setText(defaultPlatformEditor());
    m_editorEdit->setPlaceholderText(tr("Default (%1)").arg(defaultPlatformEditor()));
    m_browseEditorButton = new QPushButton(tr("Browse..."), optionsGroupBox);
    editorLayout->addWidget(m_editorEdit);
    editorLayout->addWidget(m_browseEditorButton);
    formLayout->addRow(tr("Editor:"), editorLayout);

    // Pager (POWWOWPAGER) picker with platform default placeholder
    QHBoxLayout *pagerLayout = new QHBoxLayout();
    m_pagerEdit = new QLineEdit(optionsGroupBox);
    m_pagerEdit->setText(defaultPlatformPager());
    m_pagerEdit->setPlaceholderText(tr("Default (%1)").arg(defaultPlatformPager()));
    m_browsePagerButton = new QPushButton(tr("Browse..."), optionsGroupBox);
    pagerLayout->addWidget(m_pagerEdit);
    pagerLayout->addWidget(m_browsePagerButton);
    formLayout->addRow(tr("Pager:"), pagerLayout);

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

    // Scrollback Lines configuration
    m_scrollbackSpinBox = new QSpinBox(optionsGroupBox);
    m_scrollbackSpinBox->setRange(100, 100000);
    m_scrollbackSpinBox->setSingleStep(500);
    m_scrollbackSpinBox->setValue(5000);
    formLayout->addRow(tr("Scrollback Lines:"), m_scrollbackSpinBox);

    m_tlsCheckBox = new QCheckBox(tr("Enable TLS Encryption"), optionsGroupBox);
    m_tlsCheckBox->setChecked(false);
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

    // Connect signals
    connect(m_sessionListWidget, &QListWidget::currentRowChanged, this, &MainWindow::onSessionSelected);
    connect(m_browsePowButton, &QPushButton::clicked, this, &MainWindow::onBrowsePowFile);
    connect(m_browseEditorButton, &QPushButton::clicked, this, &MainWindow::onBrowseEditor);
    connect(m_browsePagerButton, &QPushButton::clicked, this, &MainWindow::onBrowsePager);
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::onSaveSession);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteSession);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectSession);
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
        m_stackedWidget->removeWidget(m_activeTerminalWidget);
        delete m_activeTerminalWidget;
        m_activeTerminalWidget = nullptr;
    }

    SessionProfile profile = currentProfileFromUi();

    m_activeTerminalWidget = new TerminalWidget(profile, this);
    m_stackedWidget->addWidget(m_activeTerminalWidget);
    m_stackedWidget->setCurrentWidget(m_activeTerminalWidget);

    connect(m_activeTerminalWidget, &TerminalWidget::sessionClosed, this, &MainWindow::onEndSession);

    m_activeTerminalWidget->startSession();
}

void MainWindow::onEndSession()
{
    if (m_activeTerminalWidget) {
        m_stackedWidget->setCurrentWidget(m_configPage);
        m_stackedWidget->removeWidget(m_activeTerminalWidget);
        delete m_activeTerminalWidget;
        m_activeTerminalWidget = nullptr;
    }
}

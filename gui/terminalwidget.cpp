#include "terminalwidget.h"
#include <QRegularExpression>
#include <QScrollBar>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QDebug>

TerminalWidget::TerminalWidget(const SessionProfile &profile, QWidget *parent)
    : QWidget(parent),
      m_profile(profile),
      m_terminalDisplay(new QPlainTextEdit(this)),
      m_disconnectButton(new QPushButton(tr("Disconnect"), this)),
      m_process(new QProcess(this))
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->addStretch();
    topBar->addWidget(m_disconnectButton);
    mainLayout->addLayout(topBar);

    m_terminalDisplay->setReadOnly(true);
    m_terminalDisplay->setUndoRedoEnabled(false);
    m_terminalDisplay->setFocusPolicy(Qt::StrongFocus);
    m_terminalDisplay->installEventFilter(this);
    m_terminalDisplay->setMaximumBlockCount(m_profile.scrollbackLines > 0 ? m_profile.scrollbackLines : 5000);

    // Default font and black background
    QFont font("Courier New", 10);
    font.setStyleHint(QFont::Monospace);
    m_terminalDisplay->setFont(font);

    QPalette p = m_terminalDisplay->palette();
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, Qt::lightGray);
    m_terminalDisplay->setPalette(p);

    m_defaultFormat.setForeground(QColor(192, 192, 192)); // Light gray
    m_defaultFormat.setBackground(Qt::black);
    m_currentFormat = m_defaultFormat;

    mainLayout->addWidget(m_terminalDisplay);

    connect(m_disconnectButton, &QPushButton::clicked, this, &TerminalWidget::stopSession);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &TerminalWidget::onProcessReadyRead);
    connect(m_process, &QProcess::readyReadStandardError, this, &TerminalWidget::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &TerminalWidget::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &TerminalWidget::onProcessError);
}

TerminalWidget::~TerminalWidget()
{
    stopSession();
}

QString TerminalWidget::findPowwowExecutable() const
{
    QString appDir = QCoreApplication::applicationDirPath();

    // 1. Same directory as powwow-gui executable
#ifdef Q_OS_WIN
    QString localExe = appDir + "/powwow.exe";
#else
    QString localExe = appDir + "/powwow";
#endif
    if (QFileInfo::exists(localExe)) {
        return localExe;
    }

    // 2. Look in build / relative directories
#ifdef Q_OS_WIN
    if (QFileInfo::exists(appDir + "/../powwow.exe")) return appDir + "/../powwow.exe";
#else
    if (QFileInfo::exists(appDir + "/../powwow")) return appDir + "/../powwow";
    if (QFileInfo::exists(appDir + "/powwow")) return appDir + "/powwow";
#endif

    // 3. Fallback to system PATH
#ifdef Q_OS_WIN
    return "powwow.exe";
#else
    return "powwow";
#endif
}

void TerminalWidget::startSession()
{
    if (m_process->state() != QProcess::NotRunning) return;

    updateGridSize();

    QString host = m_profile.host.isEmpty() ? "mume.org" : m_profile.host;
    uint16_t port = m_profile.port > 0 ? m_profile.port : 4242;

    appendAnsiText(QString("=== Connecting (%1 %2) [%3x%4] ===\n")
                   .arg(host).arg(port).arg(m_cols).arg(m_lines));

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!m_profile.editor.isEmpty()) {
        env.insert("POWWOWEDITOR", m_profile.editor);
        env.insert("EDITOR", m_profile.editor);
    }
    if (!m_profile.pager.isEmpty()) {
        env.insert("POWWOWPAGER", m_profile.pager);
        env.insert("PAGER", m_profile.pager);
    }
    m_process->setProcessEnvironment(env);

    QString program = findPowwowExecutable();
    QStringList arguments;

    if (m_profile.useTls) {
        arguments << "--tls";
    }

    // Pass .pow profile file if specified and exists
    if (!m_profile.powFilePath.isEmpty() && QFileInfo::exists(m_profile.powFilePath)) {
        arguments << m_profile.powFilePath;
    }

    arguments << host << QString::number(port);

    m_process->start(program, arguments);
}

void TerminalWidget::stopSession()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(1000)) {
            m_process->kill();
        }
    }
}

void TerminalWidget::onProcessReadyRead()
{
    QByteArray data = m_process->readAllStandardOutput();
    data += m_process->readAllStandardError();
    if (!data.isEmpty()) {
        QString text = QString::fromUtf8(data);
        appendAnsiText(text);
    }
}

void TerminalWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    appendAnsiText("\n=== Session Disconnected ===\n");
}

void TerminalWidget::onProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        appendAnsiText(QString("\n=== Failed to start powwow executable (%1) ===\n").arg(findPowwowExecutable()));
    }
}

bool TerminalWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_terminalDisplay && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        sendKeyToProcess(keyEvent);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void TerminalWidget::sendKeyToProcess(QKeyEvent *keyEvent)
{
    if (m_process->state() != QProcess::Running) return;

    QByteArray data;
    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
        data = "\r\n";
    } else if (keyEvent->key() == Qt::Key_Backspace) {
        data = "\b";
    } else if (keyEvent->key() == Qt::Key_Tab) {
        data = "\t";
    } else {
        QString text = keyEvent->text();
        if (!text.isEmpty()) {
            data = text.toUtf8();
        }
    }

    if (!data.isEmpty()) {
        m_process->write(data);
    }
}

void TerminalWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateGridSize();
}

void TerminalWidget::updateGridSize()
{
    QFontMetrics fm(m_terminalDisplay->font());
    int charWidth = fm.horizontalAdvance('W');
    int lineHeight = fm.height();

    if (charWidth > 0 && lineHeight > 0) {
        int w = m_terminalDisplay->viewport()->width();
        int h = m_terminalDisplay->viewport()->height();
        m_cols = qMax(20, w / charWidth);
        m_lines = qMax(5, h / lineHeight);
    }
}

void TerminalWidget::appendAnsiText(const QString &text)
{
    QTextCursor cursor(m_terminalDisplay->document());
    cursor.movePosition(QTextCursor::End);

    // ANSI escape sequence regex: \x1B\[[0-9;]*[a-zA-Z]
    static const QRegularExpression regex("\x1B\\[([0-9;]*)([a-zA-Z])");

    int pos = 0;
    while (pos < text.length()) {
        int matchStart = text.length();
        int matchLen = 0;
        QRegularExpressionMatch match = regex.match(text, pos);
        if (match.hasMatch()) {
            matchStart = match.capturedStart();
            matchLen = match.capturedLength();
        }

        // Output raw text before escape sequence
        while (pos < matchStart) {
            if (text.at(pos) == '\r') {
                if (pos + 1 < text.length() && text.at(pos + 1) == '\n') {
                    // Skip \r when followed by \n; the upcoming \n will handle insertion
                } else {
                    cursor.movePosition(QTextCursor::StartOfLine);
                }
            } else if (text.at(pos) == '\n') {
                cursor.movePosition(QTextCursor::End);
                cursor.insertText("\n", m_currentFormat);
            } else if (text.at(pos) == '\b') {
                cursor.deletePreviousChar();
            } else {
                cursor.movePosition(QTextCursor::End);
                cursor.insertText(text.at(pos), m_currentFormat);
            }
            pos++;
        }

        if (match.hasMatch() && pos == matchStart) {
            QString codeStr = match.captured(1);
            QChar command = match.captured(2).at(0);

            if (command == 'm') {
                parseSgrCode(codeStr);
            }
            pos += matchLen;
        }
    }

    m_terminalDisplay->verticalScrollBar()->setValue(m_terminalDisplay->verticalScrollBar()->maximum());
}

void TerminalWidget::parseSgrCode(const QString &codeStr)
{
    if (codeStr.isEmpty() || codeStr == "0") {
        m_currentFormat = m_defaultFormat;
        return;
    }

    QStringList codes = codeStr.split(';');
    for (int i = 0; i < codes.size(); ++i) {
        int code = codes[i].toInt();
        if (code == 0) {
            m_currentFormat = m_defaultFormat;
        } else if (code == 1) {
            m_currentFormat.setFontWeight(QFont::Bold);
        } else if (code == 4) {
            m_currentFormat.setFontUnderline(true);
        } else if (code == 22) {
            m_currentFormat.setFontWeight(QFont::Normal);
        } else if (code == 24) {
            m_currentFormat.setFontUnderline(false);
        } else if (code >= 30 && code <= 37) {
            static const QColor colors[8] = {
                Qt::black, Qt::red, Qt::green, Qt::yellow,
                Qt::blue, Qt::magenta, Qt::cyan, Qt::white
            };
            m_currentFormat.setForeground(colors[code - 30]);
        } else if (code == 39) {
            m_currentFormat.setForeground(m_defaultFormat.foreground());
        } else if (code >= 40 && code <= 47) {
            static const QColor colors[8] = {
                Qt::black, Qt::red, Qt::green, Qt::yellow,
                Qt::blue, Qt::magenta, Qt::cyan, Qt::white
            };
            m_currentFormat.setBackground(colors[code - 40]);
        } else if (code == 49) {
            m_currentFormat.setBackground(m_defaultFormat.background());
        } else if (code >= 90 && code <= 97) {
            static const QColor brightColors[8] = {
                QColor(128,128,128), QColor(255,85,85), QColor(85,255,85), QColor(255,255,85),
                QColor(85,85,255), QColor(255,85,255), QColor(85,255,255), QColor(255,255,255)
            };
            m_currentFormat.setForeground(brightColors[code - 90]);
        } else if (code >= 100 && code <= 107) {
            static const QColor brightColors[8] = {
                QColor(128,128,128), QColor(255,85,85), QColor(85,255,85), QColor(255,255,85),
                QColor(85,85,255), QColor(255,85,255), QColor(85,255,255), QColor(255,255,255)
            };
            m_currentFormat.setBackground(brightColors[code - 100]);
        }
    }
}

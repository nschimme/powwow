#include "terminalwidget.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextCursor>
#include <QColor>
#include <QFontDatabase>
#include <QRegularExpression>
#include <QProcessEnvironment>
#include <QDebug>

static QString extractEmbeddedPowwow()
{
    QString resPath = ":/bin/powwow";
#ifdef Q_OS_WIN
    if (!QFile::exists(resPath)) {
        resPath = ":/bin/powwow.exe";
    }
#endif

    if (!QFile::exists(resPath)) {
        return QString();
    }

    QString tempDir = QDir::tempPath();
    QString tempBin = tempDir + "/powwow_embedded";
#ifdef Q_OS_WIN
    tempBin += ".exe";
#endif

    QFile resFile(resPath);
    if (resFile.open(QIODevice::ReadOnly)) {
        QFile outFile(tempBin);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(resFile.readAll());
            outFile.close();
            outFile.setPermissions(QFile::ExeUser | QFile::ReadUser | QFile::WriteUser |
                                  QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner);
            return tempBin;
        }
    }
    return QString();
}

TerminalWidget::TerminalWidget(const SessionProfile &profile, QWidget *parent)
    : QWidget(parent)
    , m_profile(profile)
    , m_process(new QProcess(this))
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // Top control bar (PuTTY disconnect button)
    QHBoxLayout *topLayout = new QHBoxLayout();
    m_disconnectButton = new QPushButton(tr("Disconnect"), this);
    topLayout->addStretch();
    topLayout->addWidget(m_disconnectButton);
    mainLayout->addLayout(topLayout);

    // Terminal display area (Direct PuTTY interactive display)
    m_terminalDisplay = new QTextEdit(this);
    m_terminalDisplay->setReadOnly(true);
    m_terminalDisplay->setFocusPolicy(Qt::StrongFocus);
    m_terminalDisplay->setStyleSheet("QTextEdit { background-color: #000000; color: #CCCCCC; selection-background-color: #555555; }");

    // Set monospace font
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    font.setStyleHint(QFont::Monospace);
    m_terminalDisplay->setFont(font);
    mainLayout->addWidget(m_terminalDisplay);

    // Install event filter on terminal display to capture direct key presses (PuTTY style)
    m_terminalDisplay->installEventFilter(this);

    // Connect signals
    connect(m_disconnectButton, &QPushButton::clicked, this, &TerminalWidget::stopSession);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &TerminalWidget::onProcessReadyRead);
    connect(m_process, &QProcess::readyReadStandardError, this, &TerminalWidget::onProcessReadyRead);
    connect(m_process, &QProcess::finished, this, &TerminalWidget::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &TerminalWidget::onProcessError);

    // Default format setup
    m_currentFormat.setForeground(QColor("#CCCCCC"));
    m_currentFormat.setBackground(QColor("#000000"));
}

TerminalWidget::~TerminalWidget()
{
    stopSession();
}

void TerminalWidget::startSession()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString powwowBin = appDir + "/powwow";
#ifdef Q_OS_WIN
    powwowBin += ".exe";
#endif

    if (!QFileInfo::exists(powwowBin)) {
        QString embeddedBin = extractEmbeddedPowwow();
        if (!embeddedBin.isEmpty()) {
            powwowBin = embeddedBin;
        } else {
            powwowBin = "powwow";
        }
    }

    // Configure environment variables (POWWOWEDITOR, POWWOWPAGER)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!m_profile.editor.isEmpty()) {
        env.insert("POWWOWEDITOR", m_profile.editor);
    }
    if (!m_profile.pager.isEmpty()) {
        env.insert("POWWOWPAGER", m_profile.pager);
    }
    m_process->setProcessEnvironment(env);

    QStringList args;

    if (m_profile.useTls) {
        args << "--tls";
    }
    if (m_profile.useUtf8) {
        args << "--utf8";
    } else {
        args << "--latin1";
    }

    if (!m_profile.powFilePath.isEmpty()) {
        args << m_profile.powFilePath;
    }

    if (!m_profile.host.isEmpty()) {
        args << m_profile.host;
        args << QString::number(m_profile.port);
    }

    m_terminalDisplay->append(tr("=== Connecting (%1 %2) ===").arg(m_profile.host, QString::number(m_profile.port)));

    m_process->start(powwowBin, args);
    m_terminalDisplay->setFocus();
}

void TerminalWidget::stopSession()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(1000)) {
            m_process->kill();
        }
    }
    emit sessionClosed();
}

void TerminalWidget::sendKeyToProcess(QKeyEvent *keyEvent)
{
    if (!m_process || m_process->state() == QProcess::NotRunning) {
        return;
    }

    QByteArray data;

    // Control key combinations (Ctrl + A..Z)
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        int key = keyEvent->key();
        if (key >= Qt::Key_A && key <= Qt::Key_Z) {
            char ctrlChar = static_cast<char>(key - Qt::Key_A + 1);
            data.append(ctrlChar);
            m_process->write(data);
            return;
        }
    }

    switch (keyEvent->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        data.append("\n");
        break;
    case Qt::Key_Backspace:
        data.append("\x7f");
        break;
    case Qt::Key_Tab:
        data.append("\t");
        break;
    case Qt::Key_Escape:
        data.append("\x1b");
        break;
    case Qt::Key_Up:
        data.append("\033[A");
        break;
    case Qt::Key_Down:
        data.append("\033[B");
        break;
    case Qt::Key_Right:
        data.append("\033[C");
        break;
    case Qt::Key_Left:
        data.append("\033[D");
        break;
    default:
        QString text = keyEvent->text();
        if (!text.isEmpty()) {
            data = text.toUtf8();
        }
        break;
    }

    if (!data.isEmpty()) {
        m_process->write(data);
    }
}

bool TerminalWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_terminalDisplay && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        sendKeyToProcess(keyEvent);
        return true; // Consume key press
    }
    return QWidget::eventFilter(watched, event);
}

void TerminalWidget::onProcessReadyRead()
{
    QByteArray data = m_process->readAllStandardOutput();
    data.append(m_process->readAllStandardError());
    if (!data.isEmpty()) {
        appendAnsiText(QString::fromUtf8(data));
    }
}

void TerminalWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    m_terminalDisplay->append(tr("\n=== Session Disconnected ==="));
}

void TerminalWidget::onProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        m_terminalDisplay->append(tr("\n[Error] Failed to start powwow executable. Make sure powwow is installed or in the same directory."));
    }
}

void TerminalWidget::applyAnsiCode(int code, QTextCharFormat &format)
{
    static const QColor ansiColors[8] = {
        QColor("#000000"), // Black
        QColor("#CD0000"), // Red
        QColor("#00CD00"), // Green
        QColor("#CDCD00"), // Yellow
        QColor("#0000EE"), // Blue
        QColor("#CD00CD"), // Magenta
        QColor("#00CDCD"), // Cyan
        QColor("#E5E5E5")  // White
    };
    static const QColor ansiBrightColors[8] = {
        QColor("#7F7F7F"), // Bright Black (Gray)
        QColor("#FF0000"), // Bright Red
        QColor("#00FF00"), // Bright Green
        QColor("#FFFF00"), // Bright Yellow
        QColor("#5C5CFF"), // Bright Blue
        QColor("#FF00FF"), // Bright Magenta
        QColor("#00FFFF"), // Bright Cyan
        QColor("#FFFFFF")  // Bright White
    };

    if (code == 0) {
        // Reset
        format.setForeground(QColor("#CCCCCC"));
        format.setBackground(QColor("#000000"));
        format.setFontWeight(QFont::Normal);
    } else if (code == 1) {
        // Bold
        format.setFontWeight(QFont::Bold);
    } else if (code >= 30 && code <= 37) {
        // Foreground color
        format.setForeground(ansiColors[code - 30]);
    } else if (code >= 40 && code <= 47) {
        // Background color
        format.setBackground(ansiColors[code - 40]);
    } else if (code >= 90 && code <= 97) {
        // Bright Foreground color
        format.setForeground(ansiBrightColors[code - 90]);
    } else if (code >= 100 && code <= 107) {
        // Bright Background color
        format.setBackground(ansiBrightColors[code - 100]);
    } else if (code == 39) {
        format.setForeground(QColor("#CCCCCC"));
    } else if (code == 49) {
        format.setBackground(QColor("#000000"));
    }
}

void TerminalWidget::appendAnsiText(const QString &text)
{
    QTextCursor cursor = m_terminalDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    int pos = 0;
    int len = text.length();

    static const QRegularExpression ansiRegex("\033\\[([0-9;]*)m");

    while (pos < len) {
        QRegularExpressionMatch match = ansiRegex.match(text, pos);
        int matchStart = match.hasMatch() ? match.capturedStart() : len;

        // Process plain text chunk up to matchStart
        while (pos < matchStart) {
            QChar ch = text.at(pos);
            if (ch == '\b') {
                // Backspace ("twiddler" / overwrite character)
                if (!cursor.atStart()) {
                    cursor.deletePreviousChar();
                }
            } else if (ch == '\r') {
                // Ignore bare carriage returns
            } else {
                cursor.insertText(QString(ch), m_currentFormat);
            }
            pos++;
        }

        if (match.hasMatch()) {
            // Parse SGR codes in match
            QString codeStr = match.captured(1);
            if (codeStr.isEmpty()) {
                applyAnsiCode(0, m_currentFormat);
            } else {
                const QStringList codes = codeStr.split(';');
                for (const QString &c : codes) {
                    bool ok = false;
                    int code = c.toInt(&ok);
                    if (ok) {
                        applyAnsiCode(code, m_currentFormat);
                    }
                }
            }
            pos = matchStart + match.capturedLength();
        }
    }

    // Scroll to bottom
    m_terminalDisplay->verticalScrollBar()->setValue(m_terminalDisplay->verticalScrollBar()->maximum());
}

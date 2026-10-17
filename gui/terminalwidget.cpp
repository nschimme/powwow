#include "terminalwidget.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextCursor>
#include <QColor>
#include <QFontDatabase>
#include <QRegularExpression>
#include <QDebug>

TerminalWidget::TerminalWidget(const SessionProfile &profile, QWidget *parent)
    : QWidget(parent)
    , m_profile(profile)
    , m_process(new QProcess(this))
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // Top control bar
    QHBoxLayout *topLayout = new QHBoxLayout();
    m_disconnectButton = new QPushButton(tr("Disconnect"), this);
    topLayout->addStretch();
    topLayout->addWidget(m_disconnectButton);
    mainLayout->addLayout(topLayout);

    // Terminal display area
    m_terminalDisplay = new QTextEdit(this);
    m_terminalDisplay->setReadOnly(true);
    m_terminalDisplay->setStyleSheet("QTextEdit { background-color: #000000; color: #CCCCCC; }");

    // Set monospace font
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    font.setStyleHint(QFont::Monospace);
    m_terminalDisplay->setFont(font);
    mainLayout->addWidget(m_terminalDisplay);

    // Input layout
    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_inputLine = new QLineEdit(this);
    m_inputLine->setFont(font);
    m_inputLine->setPlaceholderText(tr("Type command here..."));
    m_sendButton = new QPushButton(tr("Send"), this);

    inputLayout->addWidget(m_inputLine);
    inputLayout->addWidget(m_sendButton);
    mainLayout->addLayout(inputLayout);

    // Filter events on input line for command history (Up/Down arrows)
    m_inputLine->installEventFilter(this);

    // Connect signals
    connect(m_sendButton, &QPushButton::clicked, this, &TerminalWidget::onSendInput);
    connect(m_inputLine, &QLineEdit::returnPressed, this, &TerminalWidget::onSendInput);
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
        // Fallback to searching in PATH or working dir
        powwowBin = "powwow";
    }

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
    m_inputLine->setFocus();
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

void TerminalWidget::onSendInput()
{
    QString text = m_inputLine->text();
    if (text.isEmpty()) {
        text = "";
    }

    // Add to history
    if (!text.isEmpty()) {
        if (m_history.isEmpty() || m_history.last() != text) {
            m_history.append(text);
        }
    }
    m_historyIndex = -1;

    // Send to process
    if (m_process && m_process->isOpen()) {
        QByteArray encoded = (text + "\n").toUtf8();
        m_process->write(encoded);
    }

    m_inputLine->clear();
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

bool TerminalWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_inputLine && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Up) {
            if (!m_history.isEmpty()) {
                if (m_historyIndex == -1) {
                    m_historyIndex = m_history.size() - 1;
                } else if (m_historyIndex > 0) {
                    m_historyIndex--;
                }
                m_inputLine->setText(m_history.at(m_historyIndex));
            }
            return true;
        } else if (keyEvent->key() == Qt::Key_Down) {
            if (m_historyIndex != -1) {
                if (m_historyIndex < m_history.size() - 1) {
                    m_historyIndex++;
                    m_inputLine->setText(m_history.at(m_historyIndex));
                } else {
                    m_historyIndex = -1;
                    m_inputLine->clear();
                }
            }
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
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
        if (match.hasMatch()) {
            int matchStart = match.capturedStart();

            // Append plain text before escape code
            if (matchStart > pos) {
                QString plainText = text.mid(pos, matchStart - pos);
                // Strip out remaining control chars like \r
                plainText.remove('\r');
                cursor.insertText(plainText, m_currentFormat);
            }

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
        } else {
            // Append remaining text
            QString plainText = text.mid(pos);
            plainText.remove('\r');
            cursor.insertText(plainText, m_currentFormat);
            break;
        }
    }

    // Scroll to bottom
    m_terminalDisplay->verticalScrollBar()->setValue(m_terminalDisplay->verticalScrollBar()->maximum());
}

#include "terminalwidget.h"
#include <QRegularExpression>
#include <QScrollBar>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QGuiApplication>
#include <QClipboard>
#include <QDebug>

// Telnet Protocol Constants
static const uint8_t TN_SE   = 240;
static const uint8_t TN_GA   = 249;
static const uint8_t TN_SB   = 250;
static const uint8_t TN_WILL = 251;
static const uint8_t TN_WONT = 252;
static const uint8_t TN_DO   = 253;
static const uint8_t TN_DONT = 254;
static const uint8_t TN_IAC  = 255;

static const uint8_t TELOPT_ECHO   = 1;
static const uint8_t TELOPT_SGA    = 3;
static const uint8_t TELOPT_TTYPE  = 24;
static const uint8_t TELOPT_NAWS   = 31;
static const uint8_t TELOPT_CHARSET= 42;

TerminalWidget::TerminalWidget(const SessionProfile &profile, QWidget *parent)
    : QWidget(parent),
      m_profile(profile),
      m_terminalDisplay(new QPlainTextEdit(this))
{
    if (m_profile.useTls) {
        m_socket = new QSslSocket(this);
    } else {
        m_socket = new QTcpSocket(this);
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

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

    connect(m_socket, &QTcpSocket::readyRead, this, &TerminalWidget::onSocketReadyRead);
    connect(m_socket, &QTcpSocket::connected, this, &TerminalWidget::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TerminalWidget::onSocketDisconnected);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &TerminalWidget::onSocketError);
}

TerminalWidget::~TerminalWidget()
{
    stopSession();
}

void TerminalWidget::startSession()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) return;

    updateGridSize();

    QString host = m_profile.host.isEmpty() ? "mume.org" : m_profile.host;
    uint16_t port = m_profile.port > 0 ? m_profile.port : 4242;

    appendAnsiText(QString("=== Connecting to %1:%2 [%3x%4] ===\n")
                   .arg(host).arg(port).arg(m_cols).arg(m_lines));

    if (m_profile.useTls) {
        QSslSocket *sslSock = qobject_cast<QSslSocket*>(m_socket);
        if (sslSock) {
            sslSock->connectToHostEncrypted(host, port);
        } else {
            m_socket->connectToHost(host, port);
        }
    } else {
        m_socket->connectToHost(host, port);
    }
}

void TerminalWidget::stopSession()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

void TerminalWidget::onSocketConnected()
{
    appendAnsiText("\n*** Connected to server ***\n\n");
    sendWindowSize();
}

void TerminalWidget::onSocketDisconnected()
{
    appendAnsiText("\n=== Session Disconnected ===\n");
    emit sessionClosed();
}

void TerminalWidget::onSocketError()
{
    appendAnsiText(QString("\n=== Socket Error: %1 ===\n").arg(m_socket->errorString()));
}

void TerminalWidget::sendTelnetOption(uint8_t command, uint8_t option)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) return;
    const uint8_t buf[3] = { TN_IAC, command, option };
    m_socket->write(reinterpret_cast<const char*>(buf), 3);
}

void TerminalWidget::sendWindowSize()
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) return;
    const uint8_t buf[9] = {
        TN_IAC, TN_SB, TELOPT_NAWS,
        static_cast<uint8_t>(m_cols >> 8), static_cast<uint8_t>(m_cols & 0xFF),
        static_cast<uint8_t>(m_lines >> 8), static_cast<uint8_t>(m_lines & 0xFF),
        TN_IAC, TN_SE
    };
    m_socket->write(reinterpret_cast<const char*>(buf), 9);
}

void TerminalWidget::processTelnetStream(const QByteArray &data)
{
    QByteArray cleanData;
    cleanData.reserve(data.size());

    for (int i = 0; i < data.size(); ++i) {
        uint8_t b = static_cast<uint8_t>(data.at(i));

        switch (m_telnetState) {
        case TN_NORMAL:
            if (b == TN_IAC) {
                m_telnetState = TN_IAC;
            } else {
                cleanData.append(static_cast<char>(b));
            }
            break;

        case TN_IAC:
            if (b == TN_IAC) {
                cleanData.append(static_cast<char>(TN_IAC));
                m_telnetState = TN_NORMAL;
            } else if (b == TN_WILL) {
                m_telnetState = TN_WILL;
            } else if (b == TN_WONT) {
                m_telnetState = TN_WONT;
            } else if (b == TN_DO) {
                m_telnetState = TN_DO;
            } else if (b == TN_DONT) {
                m_telnetState = TN_DONT;
            } else if (b == TN_SB) {
                m_suboptBuffer.clear();
                m_telnetState = TN_SB;
            } else {
                m_telnetState = TN_NORMAL;
            }
            break;

        case TN_WILL:
            if (b == TELOPT_ECHO || b == TELOPT_SGA) {
                sendTelnetOption(TN_DO, b);
            } else {
                sendTelnetOption(TN_DONT, b);
            }
            m_telnetState = TN_NORMAL;
            break;

        case TN_WONT:
            sendTelnetOption(TN_DONT, b);
            m_telnetState = TN_NORMAL;
            break;

        case TN_DO:
            if (b == TELOPT_NAWS) {
                sendTelnetOption(TN_WILL, b);
                sendWindowSize();
            } else if (b == TELOPT_TTYPE) {
                sendTelnetOption(TN_WILL, b);
            } else if (b == TELOPT_SGA) {
                sendTelnetOption(TN_WILL, b);
            } else {
                sendTelnetOption(TN_WONT, b);
            }
            m_telnetState = TN_NORMAL;
            break;

        case TN_DONT:
            sendTelnetOption(TN_WONT, b);
            m_telnetState = TN_NORMAL;
            break;

        case TN_SB:
            if (b == TN_IAC) {
                m_telnetState = TN_SB_IAC;
            } else {
                m_suboptBuffer.append(static_cast<char>(b));
            }
            break;

        case TN_SB_IAC:
            if (b == TN_SE) {
                // Respond to Terminal-Type subnegotiation request SEND (1)
                if (m_suboptBuffer.size() >= 2 && static_cast<uint8_t>(m_suboptBuffer.at(0)) == TELOPT_TTYPE && static_cast<uint8_t>(m_suboptBuffer.at(1)) == 1) {
                    const uint8_t ttypeResp[] = {
                        TN_IAC, TN_SB, TELOPT_TTYPE, 0, // IS (0)
                        'P', 'O', 'W', 'W', 'O', 'W',
                        TN_IAC, TN_SE
                    };
                    m_socket->write(reinterpret_cast<const char*>(ttypeResp), sizeof(ttypeResp));
                }
                m_suboptBuffer.clear();
                m_telnetState = TN_NORMAL;
            } else if (b == TN_IAC) {
                m_suboptBuffer.append(static_cast<char>(TN_IAC));
                m_telnetState = TN_SB;
            } else {
                m_telnetState = TN_SB;
            }
            break;
        }
    }

    if (!cleanData.isEmpty()) {
        QString text = QString::fromUtf8(cleanData);
        appendAnsiText(text);
    }
}

void TerminalWidget::onSocketReadyRead()
{
    QByteArray data = m_socket->readAll();
    if (!data.isEmpty()) {
        processTelnetStream(data);
    }
}

bool TerminalWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_terminalDisplay && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        // Allow standard Copy (Ctrl+C / Cmd+C), SelectAll (Ctrl+A / Cmd+A)
        if (keyEvent->matches(QKeySequence::Copy) ||
            keyEvent->matches(QKeySequence::SelectAll)) {
            return QWidget::eventFilter(watched, event);
        }

        // Handle Paste (Ctrl+V / Cmd+V) to send clipboard text to server
        if (keyEvent->matches(QKeySequence::Paste)) {
            if (m_socket->state() == QAbstractSocket::ConnectedState) {
                QString clipText = QGuiApplication::clipboard()->text();
                if (!clipText.isEmpty()) {
                    m_socket->write(clipText.toUtf8());
                }
            }
            return true;
        }

        sendKeyToSocket(keyEvent);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void TerminalWidget::sendKeyToSocket(QKeyEvent *keyEvent)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) return;

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
        m_socket->write(data);
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

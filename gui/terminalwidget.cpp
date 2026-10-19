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
static const uint8_t TELNET_SE   = 240;
static const uint8_t TELNET_NOP  = 241;
static const uint8_t TELNET_DM   = 242;
static const uint8_t TELNET_BRK  = 243;
static const uint8_t TELNET_IP   = 244;
static const uint8_t TELNET_AO   = 245;
static const uint8_t TELNET_AYT  = 246;
static const uint8_t TELNET_EC   = 247;
static const uint8_t TELNET_EL   = 248;
static const uint8_t TELNET_GA   = 249;
static const uint8_t TELNET_SB   = 250;
static const uint8_t TELNET_WILL = 251;
static const uint8_t TELNET_WONT = 252;
static const uint8_t TELNET_DO   = 253;
static const uint8_t TELNET_DONT = 254;
static const uint8_t TELNET_IAC  = 255;

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

    // Profile Font
    QString family = m_profile.fontFamily.isEmpty() ? "PowTTY Mono" : m_profile.fontFamily;
    int size = m_profile.fontSize > 0 ? m_profile.fontSize : 10;
    QFont font(family, size);
    font.setStyleHint(QFont::Monospace);
    m_terminalDisplay->setFont(font);

    QPalette p = m_terminalDisplay->palette();
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, Qt::lightGray);
    m_terminalDisplay->setPalette(p);

    m_defaultFormat.setForeground(QColor(192, 192, 192)); // Light gray
    m_defaultFormat.setBackground(Qt::black);
    m_defaultFormat.setFont(font);
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
    const uint8_t buf[3] = { TELNET_IAC, command, option };
    m_socket->write(reinterpret_cast<const char*>(buf), 3);
}

void TerminalWidget::sendWindowSize()
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) return;
    const uint8_t buf[9] = {
        TELNET_IAC, TELNET_SB, TELOPT_NAWS,
        static_cast<uint8_t>(m_cols >> 8), static_cast<uint8_t>(m_cols & 0xFF),
        static_cast<uint8_t>(m_lines >> 8), static_cast<uint8_t>(m_lines & 0xFF),
        TELNET_IAC, TELNET_SE
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
        case STATE_NORMAL:
            if (b == TELNET_IAC) {
                m_telnetState = STATE_IAC;
            } else if (b == 0 || (b < 32 && b != '\n' && b != '\r' && b != '\t' && b != '\b' && b != 27)) {
                // Ignore non-printable control bytes
            } else {
                cleanData.append(static_cast<char>(b));
            }
            break;

        case STATE_IAC:
            if (b == TELNET_IAC) {
                cleanData.append(static_cast<char>(TELNET_IAC));
                m_telnetState = STATE_NORMAL;
            } else if (b == TELNET_WILL) {
                m_telnetState = STATE_WILL;
            } else if (b == TELNET_WONT) {
                m_telnetState = STATE_WONT;
            } else if (b == TELNET_DO) {
                m_telnetState = STATE_DO;
            } else if (b == TELNET_DONT) {
                m_telnetState = STATE_DONT;
            } else if (b == TELNET_SB) {
                m_suboptBuffer.clear();
                m_telnetState = STATE_SB;
            } else {
                // Single-byte commands like GA (Go Ahead) or NOP
                m_telnetState = STATE_NORMAL;
            }
            break;

        case STATE_WILL:
            if (b == TELOPT_ECHO) {
                // WILL ECHO: Server will echo input -> Client should turn OFF local echo
                m_localEcho = false;
                sendTelnetOption(TELNET_DO, b);
            } else if (b == TELOPT_SGA) {
                sendTelnetOption(TELNET_DO, b);
            } else {
                sendTelnetOption(TELNET_DONT, b);
            }
            m_telnetState = STATE_NORMAL;
            break;

        case STATE_WONT:
            if (b == TELOPT_ECHO) {
                // WONT ECHO: Server will NOT echo input (e.g. password prompt) -> Client local echo OFF
                m_localEcho = false;
            }
            sendTelnetOption(TELNET_DONT, b);
            m_telnetState = STATE_NORMAL;
            break;

        case STATE_DO:
            if (b == TELOPT_ECHO) {
                // DO ECHO: Server asks client to echo -> Client local echo ON
                m_localEcho = true;
                sendTelnetOption(TELNET_WILL, b);
            } else if (b == TELOPT_NAWS) {
                sendTelnetOption(TELNET_WILL, b);
                sendWindowSize();
            } else if (b == TELOPT_TTYPE) {
                sendTelnetOption(TELNET_WILL, b);
            } else if (b == TELOPT_SGA) {
                sendTelnetOption(TELNET_WILL, b);
            } else {
                sendTelnetOption(TELNET_WONT, b);
            }
            m_telnetState = STATE_NORMAL;
            break;

        case STATE_DONT:
            if (b == TELOPT_ECHO) {
                m_localEcho = false;
            }
            sendTelnetOption(TELNET_WONT, b);
            m_telnetState = STATE_NORMAL;
            break;

        case STATE_SB:
            if (b == TELNET_IAC) {
                m_telnetState = STATE_SB_IAC;
            } else {
                m_suboptBuffer.append(static_cast<char>(b));
            }
            break;

        case STATE_SB_IAC:
            if (b == TELNET_SE) {
                if (m_suboptBuffer.size() >= 2 && static_cast<uint8_t>(m_suboptBuffer.at(0)) == TELOPT_TTYPE && static_cast<uint8_t>(m_suboptBuffer.at(1)) == 1) {
                    const uint8_t ttypeResp[] = {
                        TELNET_IAC, TELNET_SB, TELOPT_TTYPE, 0,
                        'P', 'O', 'W', 'T', 'T', 'Y',
                        TELNET_IAC, TELNET_SE
                    };
                    m_socket->write(reinterpret_cast<const char*>(ttypeResp), sizeof(ttypeResp));
                }
                m_suboptBuffer.clear();
                m_telnetState = STATE_NORMAL;
            } else if (b == TELNET_IAC) {
                m_suboptBuffer.append(static_cast<char>(TELNET_IAC));
                m_telnetState = STATE_SB;
            } else {
                m_telnetState = STATE_SB;
            }
            break;
        }
    }

    if (!cleanData.isEmpty()) {
        QString text;
        if (m_profile.useUtf8) {
            m_utf8Buffer.append(cleanData);
            // Decode complete UTF-8 sequences and buffer incomplete tail bytes
            QString decoded = QString::fromUtf8(m_utf8Buffer);
            int replacementIdx = decoded.indexOf(QChar(0xFFFD));

            if (replacementIdx != -1) {
                // If incomplete multi-byte sequence at end, retain trailing incomplete bytes
                if (replacementIdx == decoded.length() - 1) {
                    m_utf8Buffer = m_utf8Buffer.right(1);
                    text = decoded.left(replacementIdx);
                } else {
                    // Fall back to Latin-1
                    text = QString::fromLatin1(m_utf8Buffer);
                    m_utf8Buffer.clear();
                }
            } else {
                text = decoded;
                m_utf8Buffer.clear();
            }
        } else {
            text = QString::fromLatin1(cleanData);
        }

        if (!text.isEmpty()) {
            appendAnsiText(text);
        }
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

        if (keyEvent->matches(QKeySequence::Copy) ||
            keyEvent->matches(QKeySequence::SelectAll)) {
            return QWidget::eventFilter(watched, event);
        }

        if (keyEvent->matches(QKeySequence::Paste)) {
            if (m_socket->state() == QAbstractSocket::ConnectedState) {
                QString clipText = QGuiApplication::clipboard()->text();
                if (!clipText.isEmpty()) {
                    m_socket->write(clipText.toUtf8());
                    if (m_localEcho) {
                        appendAnsiText(clipText);
                    }
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
    QString echoText;

    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
        data = "\r\n";
        echoText = "\n";
    } else if (keyEvent->key() == Qt::Key_Backspace) {
        data = "\b";
        echoText = "\b";
    } else if (keyEvent->key() == Qt::Key_Tab) {
        data = "\t";
        echoText = "\t";
    } else {
        QString text = keyEvent->text();
        if (!text.isEmpty()) {
            data = text.toUtf8();
            echoText = text;
        }
    }

    if (!data.isEmpty()) {
        m_socket->write(data);
        if (m_localEcho && !echoText.isEmpty()) {
            appendAnsiText(echoText);
        }
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

        while (pos < matchStart) {
            QChar ch = text.at(pos);
            if (ch == '\r') {
                if (pos + 1 < text.length() && text.at(pos + 1) == '\n') {
                    // Skip \r when followed by \n
                } else {
                    cursor.movePosition(QTextCursor::StartOfLine);
                }
            } else if (ch == '\n') {
                cursor.movePosition(QTextCursor::End);
                cursor.insertText("\n", m_currentFormat);
            } else if (ch == '\b') {
                cursor.deletePreviousChar();
            } else if (ch.unicode() < 32 && ch != '\t') {
                // Skip unprintable ASCII control characters
            } else {
                cursor.movePosition(QTextCursor::End);
                cursor.insertText(ch, m_currentFormat);
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

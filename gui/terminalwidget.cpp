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

TerminalWidget::TerminalWidget(const SessionProfile &profile, QWidget *parent)
    : QWidget(parent),
      m_profile(profile),
      m_terminalDisplay(new QPlainTextEdit(this)),
      m_disconnectButton(new QPushButton(tr("Disconnect"), this))
{
    if (m_profile.useTls) {
        m_socket = new QSslSocket(this);
    } else {
        m_socket = new QTcpSocket(this);
    }

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
}

void TerminalWidget::onSocketDisconnected()
{
    appendAnsiText("\n=== Session Disconnected ===\n");
}

void TerminalWidget::onSocketError()
{
    appendAnsiText(QString("\n=== Socket Error: %1 ===\n").arg(m_socket->errorString()));
}

void TerminalWidget::onSocketReadyRead()
{
    QByteArray data = m_socket->readAll();
    if (!data.isEmpty()) {
        QString text = QString::fromUtf8(data);
        appendAnsiText(text);
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

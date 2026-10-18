#include "terminalwidget.h"
#include <QRegularExpression>
#include <QScrollBar>
#include <QDebug>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QTextEdit(parent),
      m_socket(new QTcpSocket(this)),
      m_maxScrollbackLines(5000)
{
    setReadOnly(true);
    setUndoRedoEnabled(false);
    setFocusPolicy(Qt::StrongFocus);

    // Default font and black background
    QFont font("Courier New", 10);
    font.setStyleHint(QFont::Monospace);
    setFont(font);

    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, Qt::lightGray);
    setPalette(p);

    m_defaultFormat.setForeground(QColor(192, 192, 192)); // Light gray
    m_defaultFormat.setBackground(Qt::black);
    m_currentFormat = m_defaultFormat;

    connect(m_socket, &QTcpSocket::readyRead, this, &TerminalWidget::onReadyRead);
    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        appendAnsiText("\n*** Connected to " + m_host + ":" + QString::number(m_port) + " ***\n\n");
    });
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        appendAnsiText("\n*** Disconnected from host ***\n\n");
    });
    connect(m_socket, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        appendAnsiText("\n*** Socket error: " + m_socket->errorString() + " ***\n\n");
    });
}

TerminalWidget::~TerminalWidget()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

void TerminalWidget::connectToHost(const QString &host, uint16_t port)
{
    m_host = host;
    m_port = port;

    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }

    appendAnsiText("*** Connecting to " + host + ":" + QString::number(port) + "... ***\n");
    m_socket->connectToHost(host, port);
}

void TerminalWidget::setMaxScrollbackLines(int lines)
{
    m_maxScrollbackLines = lines;
    document()->setMaximumBlockCount(lines);
}

void TerminalWidget::keyPressEvent(QKeyEvent *event)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        QTextEdit::keyPressEvent(event);
        return;
    }

    QString text = event->text();
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        m_socket->write("\r\n");
        return;
    }
    if (event->key() == Qt::Key_Backspace) {
        m_socket->write("\b");
        return;
    }
    if (event->key() == Qt::Key_Tab) {
        m_socket->write("\t");
        return;
    }

    if (!text.isEmpty()) {
        m_socket->write(text.toUtf8());
    }
}

void TerminalWidget::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    QString text = QString::fromUtf8(data);
    appendAnsiText(text);
}

void TerminalWidget::appendAnsiText(const QString &text)
{
    QTextCursor cursor(document());
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
                // Select Graphic Rendition (SGR)
                parseSgrCode(codeStr);
            }
            pos += matchLen;
        }
    }

    // Auto-scroll to bottom
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
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

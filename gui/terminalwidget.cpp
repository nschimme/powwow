#include "terminalwidget.h"
#include "mudconfigdialog.h"
#include <QToolBar>
#include <QRegularExpression>
#include <QScrollBar>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QGuiApplication>
#include <QClipboard>
#include <QLabel>
#include <QHBoxLayout>
#include <QDebug>

TerminalWidget::TerminalWidget(const SessionProfile &profile, QWidget *parent)
    : QWidget(parent),
      m_profile(profile),
      m_terminalDisplay(new QPlainTextEdit(this))
{
    powwow_ui_ops_t ops = {};
    ops.display_text = [](void *userData, const char *text, size_t len) {
        if (!userData || !text) return;
        auto *self = static_cast<TerminalWidget *>(userData);
        QString str = QString::fromUtf8(text, len);
        if (str.contains("password", Qt::CaseInsensitive) || str.contains("passcode", Qt::CaseInsensitive)) {
            self->m_passwordMode = true;
        }
        self->appendAnsiText(str);
    };
    ops.display_prompt = [](void *userData, const char *prompt) {
        if (!userData || !prompt) return;
        auto *self = static_cast<TerminalWidget *>(userData);
        QString str = QString::fromUtf8(prompt);
        self->appendAnsiText(str);
    };
    ops.connection_status = [](void *userData, bool connected, const char *host, int port) {
        if (!userData) return;
        auto *self = static_cast<TerminalWidget *>(userData);
        self->updateStatusDisplay(connected);
        if (connected) {
            self->appendAnsiText(QString("\n*** Connected to %1:%2 via libpowwow ***\n\n").arg(host).arg(port));
            if (!self->m_notifier) {
                int fd = powwow_get_connection_fd(self->m_powwowSession);
                if (fd >= 0) {
                    self->m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, self);
                    connect(self->m_notifier, &QSocketNotifier::activated, self, &TerminalWidget::onSocketActivated);
                }
            }
        } else {
            self->appendAnsiText(QString("\n=== Connection closed ===\n"));
            if (self->m_notifier) {
                self->m_notifier->setEnabled(false);
                delete self->m_notifier;
                self->m_notifier = nullptr;
            }
        }
    };

    m_powwowSession = powwow_session_create(&ops, this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Session Toolbar for quick UX flows
    QToolBar *sessionToolbar = new QToolBar(this);
    sessionToolbar->setStyleSheet("QToolBar { background: #252526; color: white; border-bottom: 1px solid #3c3c3c; padding: 4px; } QToolButton { color: white; padding: 4px 8px; border-radius: 3px; } QToolButton:hover { background-color: #383838; }");
    QAction *inspectAction = sessionToolbar->addAction(tr("⚡ MUD Config (Aliases / Triggers / Variables)"));
    inspectAction->setShortcut(QKeySequence("Ctrl+M"));
    connect(inspectAction, &QAction::triggered, this, &TerminalWidget::openInspector);

    QAction *disconnectAction = sessionToolbar->addAction(tr("🔌 Disconnect"));
    connect(disconnectAction, &QAction::triggered, this, &TerminalWidget::stopSession);

    mainLayout->addWidget(sessionToolbar);

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

    mainLayout->addWidget(m_terminalDisplay, 1);

    // Modern bottom status bar
    QWidget *statusBarWidget = new QWidget(this);
    statusBarWidget->setStyleSheet("background-color: #007acc; color: #ffffff; font-size: 11px; padding: 2px 6px;");
    QHBoxLayout *statusLayout = new QHBoxLayout(statusBarWidget);
    statusLayout->setContentsMargins(6, 2, 6, 2);

    m_statusLabel = new QLabel(tr("⚪ Disconnected"), statusBarWidget);
    m_statusLabel->setStyleSheet("font-weight: bold;");

    QString tlsInfo = m_profile.useTls ? "[TLS]" : "[Plain TCP]";
    QLabel *targetLabel = new QLabel(QString("%1:%2 %3").arg(m_profile.host.isEmpty() ? "mume.org" : m_profile.host).arg(m_profile.port > 0 ? m_profile.port : 4242).arg(tlsInfo), statusBarWidget);

    m_geoLabel = new QLabel(QString("%1x%2").arg(m_cols).arg(m_lines), statusBarWidget);

    statusLayout->addWidget(m_statusLabel);
    statusLayout->addWidget(targetLabel);
    statusLayout->addStretch(1);
    statusLayout->addWidget(m_geoLabel);

    mainLayout->addWidget(statusBarWidget);
}

void TerminalWidget::updateStatusDisplay(bool connected)
{
    if (m_statusLabel) {
        if (connected) {
            m_statusLabel->setText(tr("🟢 Connected"));
        } else {
            m_statusLabel->setText(tr("⚪ Disconnected"));
        }
    }
}

TerminalWidget::~TerminalWidget()
{
    stopSession();
    if (m_powwowSession) {
        powwow_session_destroy(m_powwowSession);
        m_powwowSession = nullptr;
    }
}

void TerminalWidget::startSession()
{
    updateGridSize();

    QString host = m_profile.host.isEmpty() ? "mume.org" : m_profile.host;
    uint16_t port = m_profile.port > 0 ? m_profile.port : 4242;

    appendAnsiText(QString("=== Connecting to %1:%2 [%3x%4] via libpowwow ===\n")
                   .arg(host).arg(port).arg(m_cols).arg(m_lines));

    if (m_powwowSession) {
        powwow_set_window_size(m_powwowSession, m_cols, m_lines);
        int res = powwow_connect(m_powwowSession, host.toUtf8().constData(), port, m_profile.useTls);
        if (res >= 0) {
            int fd = powwow_get_connection_fd(m_powwowSession);
            if (fd >= 0) {
                m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
                connect(m_notifier, &QSocketNotifier::activated, this, &TerminalWidget::onSocketActivated);
            }
        } else {
            appendAnsiText(QString("\n=== Failed to connect to %1:%2 ===\n").arg(host).arg(port));
        }
    }
}

void TerminalWidget::stopSession()
{
    if (m_notifier) {
        m_notifier->setEnabled(false);
        delete m_notifier;
        m_notifier = nullptr;
    }
    if (m_powwowSession && powwow_is_connected(m_powwowSession)) {
        powwow_disconnect(m_powwowSession);
    } else {
        updateStatusDisplay(false);
    }
}

void TerminalWidget::onSocketActivated(int socket)
{
    Q_UNUSED(socket);
    if (!m_powwowSession || !powwow_is_connected(m_powwowSession)) {
        stopSession();
        return;
    }
    powwow_process_network(m_powwowSession);
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
            QString clipText = QGuiApplication::clipboard()->text();
            if (!clipText.isEmpty() && m_powwowSession) {
                powwow_send_line(m_powwowSession, clipText.toUtf8().constData());
                QString displayMask = m_passwordMode ? QString("*").repeated(clipText.length()) : clipText;
                appendAnsiText(displayMask);
            }
            return true;
        }

        sendKeyToSession(keyEvent);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void TerminalWidget::openInspector()
{
    MudConfigDialog dlg(m_powwowSession, this);
    dlg.exec();
}

void TerminalWidget::sendKeyToSession(QKeyEvent *keyEvent)
{
    if (!m_powwowSession) return;

    QString echoText;

    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
        QString lineToExecute = m_inputLineBuffer;
        m_inputLineBuffer.clear();

        appendAnsiText("\n");
        powwow_add_history(m_powwowSession, lineToExecute.toUtf8().constData());
        powwow_send_line(m_powwowSession, lineToExecute.toUtf8().constData());
        m_passwordMode = false;
        return;
    } else if (keyEvent->key() == Qt::Key_Backspace) {
        if (!m_inputLineBuffer.isEmpty()) {
            m_inputLineBuffer.chop(1);
        }
        echoText = "\b";
    } else if (keyEvent->key() == Qt::Key_Tab) {
        echoText = "\t";
    } else {
        QString text = keyEvent->text();
        if (!text.isEmpty()) {
            m_inputLineBuffer.append(text);
            if (m_passwordMode) {
                echoText = "*";
            } else {
                echoText = text;
            }
        }
    }

    if (!echoText.isEmpty()) {
        appendAnsiText(echoText);
    }
}

void TerminalWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateGridSize();
    if (m_powwowSession) {
        powwow_set_window_size(m_powwowSession, m_cols, m_lines);
    }
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
    if (m_geoLabel) {
        m_geoLabel->setText(QString("%1x%2").arg(m_cols).arg(m_lines));
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

#include "terminalwidget.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextBlock>
#include <QColor>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QRegularExpression>
#include <QProcessEnvironment>
#include <QResizeEvent>
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

static QColor colorFrom256Index(int index)
{
    static const QColor baseColors[16] = {
        QColor("#000000"), QColor("#CD0000"), QColor("#00CD00"), QColor("#CDCD00"),
        QColor("#0000EE"), QColor("#CD00CD"), QColor("#00CDCD"), QColor("#E5E5E5"),
        QColor("#7F7F7F"), QColor("#FF0000"), QColor("#00FF00"), QColor("#FFFF00"),
        QColor("#5C5CFF"), QColor("#FF00FF"), QColor("#00FFFF"), QColor("#FFFFFF")
    };

    if (index >= 0 && index < 16) {
        return baseColors[index];
    } else if (index >= 16 && index <= 231) {
        int idx = index - 16;
        int r = (idx / 36);
        int g = (idx % 36) / 6;
        int b = (idx % 6);
        static const int valMap[6] = { 0, 95, 135, 175, 215, 255 };
        return QColor(valMap[r], valMap[g], valMap[b]);
    } else if (index >= 232 && index <= 255) {
        int gray = 8 + (index - 232) * 10;
        return QColor(gray, gray, gray);
    }
    return QColor("#CCCCCC");
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

    // Terminal display area using QPlainTextEdit (NoWrap for fixed grid alignment)
    m_terminalDisplay = new QPlainTextEdit(this);
    m_terminalDisplay->setReadOnly(true);
    m_terminalDisplay->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_terminalDisplay->setFocusPolicy(Qt::StrongFocus);
    m_terminalDisplay->setStyleSheet("QPlainTextEdit { background-color: #000000; color: #CCCCCC; selection-background-color: #555555; }");

    // Set monospace font for fixed character grid
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    font.setStyleHint(QFont::Monospace);
    m_terminalDisplay->setFont(font);
    mainLayout->addWidget(m_terminalDisplay);

    // Install event filter on terminal display for direct key inputs (PuTTY style)
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

    updateGridSize();
}

TerminalWidget::~TerminalWidget()
{
    stopSession();
}

void TerminalWidget::updateGridSize()
{
    QFontMetrics fm(m_terminalDisplay->font());
    int charWidth = fm.horizontalAdvance('M');
    int lineHeight = fm.height();

    if (charWidth > 0 && lineHeight > 0) {
        m_cols = qMax(20, m_terminalDisplay->viewport()->width() / charWidth);
        m_lines = qMax(5, m_terminalDisplay->viewport()->height() / lineHeight);
    }
}

void TerminalWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateGridSize();
}

void TerminalWidget::startSession()
{
    updateGridSize();

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

    // Configure environment variables (POWWOWEDITOR, POWWOWPAGER, COLUMNS, LINES for NAWS)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!m_profile.editor.isEmpty()) {
        env.insert("POWWOWEDITOR", m_profile.editor);
    }
    if (!m_profile.pager.isEmpty()) {
        env.insert("POWWOWPAGER", m_profile.pager);
    }
    env.insert("COLUMNS", QString::number(m_cols));
    env.insert("LINES", QString::number(m_lines));

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

    m_terminalDisplay->appendPlainText(tr("=== Connecting (%1 %2) [%3x%4] ===").arg(m_profile.host, QString::number(m_profile.port), QString::number(m_cols), QString::number(m_lines)));

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
    m_terminalDisplay->appendPlainText(tr("\n=== Session Disconnected ==="));
}

void TerminalWidget::onProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        m_terminalDisplay->appendPlainText(tr("\n[Error] Failed to start powwow executable. Make sure powwow is installed or in the same directory."));
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
        format.setFontUnderline(false);
    } else if (code == 1) {
        // Bold / High intensity
        format.setFontWeight(QFont::Bold);
    } else if (code == 4) {
        // Underline
        format.setFontUnderline(true);
    } else if (code == 7) {
        // Reverse / Inverse video
        QColor fg = format.foreground().color();
        QColor bg = format.background().color();
        format.setForeground(bg);
        format.setBackground(fg);
    } else if (code >= 30 && code <= 37) {
        // Standard Foreground color
        format.setForeground(ansiColors[code - 30]);
    } else if (code >= 40 && code <= 47) {
        // Standard Background color
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
    // Always move write cursor to the end of text document before appending new output
    QTextCursor cursor = m_terminalDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    int pos = 0;
    int len = text.length();

    // Match all CSI ANSI sequences \033[ ... <letter>
    static const QRegularExpression ansiRegex("\033\\[([0-9;]*)([a-zA-Z])");

    while (pos < len) {
        QRegularExpressionMatch match = ansiRegex.match(text, pos);
        int matchStart = match.hasMatch() ? match.capturedStart() : len;

        // Process plain text chunk up to matchStart
        while (pos < matchStart) {
            QChar ch = text.at(pos);
            if (ch == '\b') {
                // Backspace ("twiddler" / overwrite character)
                // Only delete character within current line/block
                if (cursor.positionInBlock() > 0) {
                    cursor.deletePreviousChar();
                }
            } else if (ch == '\r') {
                // Check if followed by \n (standard \r\n newline)
                if (pos + 1 < matchStart && text.at(pos + 1) == '\n') {
                    pos++;
                    cursor.movePosition(QTextCursor::End);
                    cursor.insertText("\n", m_currentFormat);
                } else {
                    // Bare Carriage Return: move cursor to start of block for prompt/status overwrite
                    cursor.movePosition(QTextCursor::StartOfBlock);
                }
            } else if (ch == '\n') {
                cursor.movePosition(QTextCursor::End);
                cursor.insertText("\n", m_currentFormat);
            } else {
                cursor.insertText(QString(ch), m_currentFormat);
            }
            pos++;
        }

        if (match.hasMatch()) {
            QString codeStr = match.captured(1);
            QString cmd = match.captured(2);

            if (cmd == "m") {
                // SGR formatting & colors
                if (codeStr.isEmpty()) {
                    applyAnsiCode(0, m_currentFormat);
                } else {
                    const QStringList codes = codeStr.split(';');
                    for (int i = 0; i < codes.size(); ++i) {
                        int code = codes.at(i).toInt();
                        // 256-color support (38;5;n or 48;5;n)
                        if ((code == 38 || code == 48) && i + 2 < codes.size() && codes.at(i + 1).toInt() == 5) {
                            int colorIdx = codes.at(i + 2).toInt();
                            QColor c = colorFrom256Index(colorIdx);
                            if (code == 38) {
                                m_currentFormat.setForeground(c);
                            } else {
                                m_currentFormat.setBackground(c);
                            }
                            i += 2;
                        }
                        // TrueColor 24-bit support (38;2;r;g;b or 48;2;r;g;b)
                        else if ((code == 38 || code == 48) && i + 4 < codes.size() && codes.at(i + 1).toInt() == 2) {
                            int r = codes.at(i + 2).toInt();
                            int g = codes.at(i + 3).toInt();
                            int b = codes.at(i + 4).toInt();
                            QColor c(r, g, b);
                            if (code == 38) {
                                m_currentFormat.setForeground(c);
                            } else {
                                m_currentFormat.setBackground(c);
                            }
                            i += 4;
                        } else {
                            applyAnsiCode(code, m_currentFormat);
                        }
                    }
                }
            } else if (cmd == "J") {
                // Clear screen / display
                int p = codeStr.toInt();
                if (p == 2 || p == 0) {
                    m_terminalDisplay->clear();
                    cursor = m_terminalDisplay->textCursor();
                }
            } else if (cmd == "K") {
                // Clear to end of line
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
            } else if (cmd == "H" || cmd == "f") {
                // Cursor position \033[<line>;<col>H
                const QStringList params = codeStr.split(';');
                int line = params.size() > 0 ? qMax(1, params.at(0).toInt()) : 1;
                int col = params.size() > 1 ? qMax(1, params.at(1).toInt()) : 1;

                QTextBlock block = m_terminalDisplay->document()->findBlockByLineNumber(line - 1);
                if (block.isValid()) {
                    int p = block.position() + qMin(col - 1, block.length() - 1);
                    cursor.setPosition(p);
                }
            } else if (cmd == "A") {
                // Cursor Up
                int count = qMax(1, codeStr.toInt());
                cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor, count);
            } else if (cmd == "B") {
                // Cursor Down
                int count = qMax(1, codeStr.toInt());
                cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, count);
            } else if (cmd == "C") {
                // Cursor Right
                int count = qMax(1, codeStr.toInt());
                cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, count);
            } else if (cmd == "D") {
                // Cursor Left
                int count = qMax(1, codeStr.toInt());
                cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, count);
            }

            pos = matchStart + match.capturedLength();
        }
    }

    // Scroll to bottom
    m_terminalDisplay->verticalScrollBar()->setValue(m_terminalDisplay->verticalScrollBar()->maximum());
}

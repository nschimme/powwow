#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QSocketNotifier>
#include <QVBoxLayout>
#include <QTextCharFormat>
#include <QStringList>

#include "profilemanager.h"
#include "powwow/powwow.h"

class TerminalWidget : public QWidget {
    Q_OBJECT
public:
    explicit TerminalWidget(const SessionProfile &profile, QWidget *parent = nullptr);
    ~TerminalWidget();

    void startSession();
    void stopSession();

    powwow_session_t *powwowSession() const { return m_powwowSession; }

    int cols() const { return m_cols; }
    int lines() const { return m_lines; }

signals:
    void sessionClosed();
    void toggleInspectorRequested();

public slots:
    void openInspector();
    void clearTerminal();
    void reconnectSession();

private slots:
    void onSocketActivated(int socket);
    void onCommandSubmitted();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void appendAnsiText(const QString &text);
    void parseSgrCode(const QString &codeStr);
    void updateGridSize();
    void updateStatusDisplay(bool connected);

    SessionProfile m_profile;
    QPlainTextEdit *m_terminalDisplay;
    QLineEdit *m_commandInput;
    QSocketNotifier *m_notifier = nullptr;

    QTextCharFormat m_defaultFormat;
    QTextCharFormat m_currentFormat;
    int m_cols = 80;
    int m_lines = 24;

    bool m_passwordMode = false;
    QStringList m_history;
    int m_historyIndex = -1;

    powwow_session_t *m_powwowSession = nullptr;

    class QLabel *m_statusLabel = nullptr;
    class QLabel *m_geoLabel = nullptr;
    class QAction *m_reconnectAction = nullptr;
    class QAction *m_disconnectAction = nullptr;
};

#endif // TERMINALWIDGET_H

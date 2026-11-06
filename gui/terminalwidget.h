#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QSocketNotifier>
#include <QVBoxLayout>
#include <QTextCharFormat>

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

private slots:
    void onSocketActivated(int socket);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void appendAnsiText(const QString &text);
    void parseSgrCode(const QString &codeStr);
    void sendKeyToSession(QKeyEvent *keyEvent);
    void updateGridSize();

    SessionProfile m_profile;
    QPlainTextEdit *m_terminalDisplay;
    QSocketNotifier *m_notifier = nullptr;

    QTextCharFormat m_defaultFormat;
    QTextCharFormat m_currentFormat;
    int m_cols = 80;
    int m_lines = 24;

    bool m_passwordMode = false;
    QString m_inputLineBuffer;

    powwow_session_t *m_powwowSession = nullptr;

public slots:
    void openInspector();
};

#endif // TERMINALWIDGET_H

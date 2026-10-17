#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QProcess>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextCharFormat>
#include <QStringList>

#include "profilemanager.h"

class TerminalWidget : public QWidget {
    Q_OBJECT
public:
    explicit TerminalWidget(const SessionProfile &profile, QWidget *parent = nullptr);
    ~TerminalWidget();

    void startSession();
    void stopSession();

signals:
    void sessionClosed();

private slots:
    void onSendInput();
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void appendAnsiText(const QString &text);
    void applyAnsiCode(int code, QTextCharFormat &format);

    SessionProfile m_profile;
    QTextEdit *m_terminalDisplay;
    QLineEdit *m_inputLine;
    QPushButton *m_sendButton;
    QPushButton *m_disconnectButton;
    QProcess *m_process;

    QStringList m_history;
    int m_historyIndex = -1;

    QTextCharFormat m_currentFormat;
};

#endif // TERMINALWIDGET_H

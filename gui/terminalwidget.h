#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QProcess>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextCharFormat>

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
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void appendAnsiText(const QString &text);
    void applyAnsiCode(int code, QTextCharFormat &format);
    void sendKeyToProcess(QKeyEvent *keyEvent);

    SessionProfile m_profile;
    QTextEdit *m_terminalDisplay;
    QPushButton *m_disconnectButton;
    QProcess *m_process;

    QTextCharFormat m_currentFormat;
};

#endif // TERMINALWIDGET_H

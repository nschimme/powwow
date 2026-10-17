#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
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

    int cols() const { return m_cols; }
    int lines() const { return m_lines; }

signals:
    void sessionClosed();

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void appendAnsiText(const QString &text);
    void applyAnsiCode(int code, QTextCharFormat &format);
    void sendKeyToProcess(QKeyEvent *keyEvent);
    void updateGridSize();

    SessionProfile m_profile;
    QPlainTextEdit *m_terminalDisplay;
    QPushButton *m_disconnectButton;
    QProcess *m_process;

    QTextCharFormat m_currentFormat;
    int m_cols = 80;
    int m_lines = 24;
};

#endif // TERMINALWIDGET_H

#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QTcpSocket>
#include <QSslSocket>
#include <QVBoxLayout>
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
    void onSocketReadyRead();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void processTelnetStream(const QByteArray &data);
    void sendTelnetOption(uint8_t command, uint8_t option);
    void sendWindowSize();
    void appendAnsiText(const QString &text);
    void parseSgrCode(const QString &codeStr);
    void sendKeyToSocket(QKeyEvent *keyEvent);
    void updateGridSize();

    SessionProfile m_profile;
    QPlainTextEdit *m_terminalDisplay;
    QTcpSocket *m_socket;

    QTextCharFormat m_defaultFormat;
    QTextCharFormat m_currentFormat;
    int m_cols = 80;
    int m_lines = 24;

    // Telnet parser state machine
    enum TelnetState {
        TN_NORMAL,
        TN_IAC,
        TN_WILL,
        TN_WONT,
        TN_DO,
        TN_DONT,
        TN_SB,
        TN_SB_IAC
    };
    TelnetState m_telnetState = TN_NORMAL;
    QByteArray m_suboptBuffer;
};

#endif // TERMINALWIDGET_H

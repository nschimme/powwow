#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QFontComboBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QSplitter>

#include "profilemanager.h"
#include "terminalwidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSessionSelected(int index);
    void onBrowsePowFile();
    void onBrowseEditor();
    void onBrowsePager();
    void onSaveSession();
    void onDeleteSession();
    void onConnectSession();
    void onEndSession();
    void onInspectStructs();
    void onNewSession();

private:
    void setupUi();
    void refreshSessionList();
    SessionProfile currentProfileFromUi() const;

    ProfileManager m_profileManager;

    QSplitter *m_mainSplitter;
    QWidget *m_sidebarWidget;
    QStackedWidget *m_contentStack;
    QWidget *m_configPage;
    TerminalWidget *m_activeTerminalWidget = nullptr;

    QListWidget *m_sessionListWidget;
    QLineEdit *m_sessionNameEdit;
    QLineEdit *m_hostEdit;
    QSpinBox *m_portSpinBox;
    QLineEdit *m_powFileEdit;
    QPushButton *m_browsePowButton;
    QLineEdit *m_editorEdit;
    QPushButton *m_browseEditorButton;
    QLineEdit *m_pagerEdit;
    QPushButton *m_browsePagerButton;
    QSpinBox *m_scrollbackSpinBox;
    QFontComboBox *m_fontComboBox;
    QSpinBox *m_fontSizeSpinBox;
    QCheckBox *m_tlsCheckBox;
    QCheckBox *m_utf8CheckBox;
    QPushButton *m_newSessionButton;
    QPushButton *m_saveButton;
    QPushButton *m_deleteButton;
    QPushButton *m_connectButton;
    QPushButton *m_inspectButton;
};

#endif // MAINWINDOW_H

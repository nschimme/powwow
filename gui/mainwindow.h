#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>

#include "profilemanager.h"

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
    void onCloseTerminalTab(int index);

private:
    void setupUi();
    void refreshSessionList();
    SessionProfile currentProfileFromUi() const;

    ProfileManager m_profileManager;

    QStackedWidget *m_stackedWidget;
    QWidget *m_configPage;
    QTabWidget *m_terminalTabWidget;

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
    QCheckBox *m_tlsCheckBox;
    QCheckBox *m_utf8CheckBox;
    QPushButton *m_saveButton;
    QPushButton *m_deleteButton;
    QPushButton *m_connectButton;
};

#endif // MAINWINDOW_H

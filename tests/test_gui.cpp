#include <QApplication>
#include <cassert>
#include "profilemanager.h"
#include "mudconfigdialog.h"
#include "terminalwidget.h"
#include "powwow/powwow.h"

int main(int argc, char *argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    printf("Testing GUI ProfileManager...\n");
    ProfileManager pm;
    SessionProfile p;
    p.name = "TestMUD";
    p.host = "mume.org";
    p.port = 4242;
    p.useTls = false;
    p.useUtf8 = true;

    pm.saveSession(p);
    QStringList sessions = pm.sessionNames();
    assert(sessions.contains("TestMUD"));

    SessionProfile loaded = pm.loadSession("TestMUD");
    assert(loaded.host == "mume.org");
    assert(loaded.port == 4242);
    pm.deleteSession("TestMUD");

    printf("Testing GUI MudConfigDialog and TerminalWidget with libpowwow...\n");
    powwow_ui_ops_t ops = {};
    powwow_session_t *s = powwow_session_create(&ops, nullptr);
    assert(s != nullptr);

    powwow_set_alias(s, "hello", "say Hello World", "chat");
    powwow_set_action(s, "^Tick", "#print TICK", "timer");
    powwow_set_variable(s, "hp", "500");

    MudConfigDialog dlg(s);
    dlg.refreshData();

    TerminalWidget term(p);
    assert(term.powwowSession() != nullptr);

    powwow_session_destroy(s);

    printf("test_gui PASSED!\n");
    return 0;
}

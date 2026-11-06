#ifdef _WIN32
#include "powwow/powwow.h"
#include <stdlib.h>
#include <string.h>

struct powwow_session {
    powwow_ui_ops_t ops;
    void *user_data;
    bool connected;
    char host[256];
    int port;
};

powwow_session_t *powwow_session_create(const powwow_ui_ops_t *ops, void *user_data) {
    powwow_session_t *s = (powwow_session_t *)calloc(1, sizeof(powwow_session_t));
    if (!s) return NULL;
    if (ops) s->ops = *ops;
    s->user_data = user_data;
    return s;
}

void powwow_session_destroy(powwow_session_t *session) {
    free(session);
}

int powwow_connect(powwow_session_t *session, const char *hostname, int port, bool use_tls) {
    (void)use_tls;
    if (!session || !hostname) return -1;
    strncpy(session->host, hostname, sizeof(session->host) - 1);
    session->port = port;
    session->connected = true;
    if (session->ops.connection_status) {
        session->ops.connection_status(session->user_data, true, hostname, port);
    }
    return 0;
}

void powwow_disconnect(powwow_session_t *session) {
    if (!session) return;
    session->connected = false;
    if (session->ops.connection_status) {
        session->ops.connection_status(session->user_data, false, session->host, session->port);
    }
}

bool powwow_is_connected(powwow_session_t *session) {
    return session ? session->connected : false;
}

void powwow_send_line(powwow_session_t *session, const char *line) {
    if (!session || !line) return;
    if (session->ops.send_server_data) {
        session->ops.send_server_data(session->user_data, line, strlen(line));
        session->ops.send_server_data(session->user_data, "\r\n", 2);
    }
}

void powwow_process_remote(powwow_session_t *session, const char *buf, size_t len) {
    if (!session || !buf) return;
    if (session->ops.display_text) {
        session->ops.display_text(session->user_data, buf, len);
    }
}

void powwow_process_input(powwow_session_t *session, const char *buf, size_t len) {
    powwow_process_remote(session, buf, len);
}

void powwow_process_network(powwow_session_t *session) {
    (void)session;
}

int powwow_get_connection_fd(powwow_session_t *session) {
    (void)session;
    return -1;
}

void powwow_set_window_size(powwow_session_t *session, int cols, int rows) {
    (void)session; (void)cols; (void)rows;
}

void powwow_list_aliases(powwow_session_t *session, powwow_alias_callback_t cb, void *user_data) {
    (void)session; (void)cb; (void)user_data;
}
int powwow_set_alias(powwow_session_t *session, const char *name, const char *subst, const char *group) {
    (void)session; (void)name; (void)subst; (void)group; return 0;
}
int powwow_delete_alias(powwow_session_t *session, const char *name) {
    (void)session; (void)name; return 0;
}

void powwow_list_actions(powwow_session_t *session, powwow_action_callback_t cb, void *user_data) {
    (void)session; (void)cb; (void)user_data;
}
int powwow_set_action(powwow_session_t *session, const char *pattern, const char *cmd, const char *group) {
    (void)session; (void)pattern; (void)cmd; (void)group; return 0;
}
int powwow_delete_action(powwow_session_t *session, const char *pattern) {
    (void)session; (void)pattern; return 0;
}

void powwow_list_variables(powwow_session_t *session, powwow_var_callback_t cb, void *user_data) {
    (void)session; (void)cb; (void)user_data;
}
int powwow_set_variable(powwow_session_t *session, const char *name, const char *val) {
    (void)session; (void)name; (void)val; return 0;
}
const char *powwow_get_variable(powwow_session_t *session, const char *name) {
    (void)session; (void)name; return NULL;
}
int powwow_delete_variable(powwow_session_t *session, const char *name) {
    (void)session; (void)name; return 0;
}

void powwow_list_binds(powwow_session_t *session, powwow_bind_callback_t cb, void *user_data) {
    (void)session; (void)cb; (void)user_data;
}
int powwow_set_bind(powwow_session_t *session, const char *name, const char *sequence, const char *command) {
    (void)session; (void)name; (void)sequence; (void)command; return 0;
}
int powwow_delete_bind(powwow_session_t *session, const char *name) {
    (void)session; (void)name; return 0;
}

void powwow_list_marks(powwow_session_t *session, powwow_mark_callback_t cb, void *user_data) {
    (void)session; (void)cb; (void)user_data;
}
int powwow_set_mark(powwow_session_t *session, const char *pattern, int attrcode, bool at_beg) {
    (void)session; (void)pattern; (void)attrcode; (void)at_beg; return 0;
}
int powwow_delete_mark(powwow_session_t *session, const char *pattern, bool at_beg) {
    (void)session; (void)pattern; (void)at_beg; return 0;
}

void powwow_list_delays(powwow_session_t *session, powwow_delay_callback_t cb, void *user_data) {
    (void)session; (void)cb; (void)user_data;
}
int powwow_delete_delay(powwow_session_t *session, const char *name) {
    (void)session; (void)name; return 0;
}

void powwow_list_history(powwow_session_t *session, powwow_history_callback_t cb, void *user_data) {
    (void)session; (void)cb; (void)user_data;
}
void powwow_add_history(powwow_session_t *session, const char *line) {
    (void)session; (void)line;
}

void powwow_list_options(powwow_session_t *session, powwow_option_callback_t cb, void *user_data) {
    (void)session; (void)cb; (void)user_data;
}
int powwow_set_option(powwow_session_t *session, const char *name, bool value) {
    (void)session; (void)name; (void)value; return 0;
}
int powwow_get_option(powwow_session_t *session, const char *name) {
    (void)session; (void)name; return 0;
}

#endif

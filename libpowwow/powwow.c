#include "powwow/powwow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "defines.h"
#include "main.h"
#include "list.h"
#include "cmd.h"
#include "edit.h"
#include "tcp.h"
#include "log.h"
#include "tty.h"
#include "utils.h"

static powwow_ui_ops_t g_ui_ops = {0};
static void *g_ui_user_data = NULL;
static bool g_initialized = false;

struct powwow_session {
    powwow_ui_ops_t ops;
    void *user_data;
    bool connected;
    char host[256];
    int port;
};

static powwow_session_t *g_current_session = NULL;

void powwow_internal_ui_display_text(const char *text, size_t len)
{
    if (g_ui_ops.display_text) {
        g_ui_ops.display_text(g_ui_user_data, text, len);
    }
}

void powwow_internal_ui_display_prompt(const char *prompt)
{
    if (g_ui_ops.display_prompt) {
        g_ui_ops.display_prompt(g_ui_user_data, prompt);
    }
}

void powwow_internal_ui_set_title(const char *title)
{
    if (g_ui_ops.set_title) {
        g_ui_ops.set_title(g_ui_user_data, title);
    }
}

void powwow_internal_ui_bell(void)
{
    if (g_ui_ops.bell) {
        g_ui_ops.bell(g_ui_user_data);
    }
}

void powwow_internal_ui_send_server_data(const char *data, size_t len)
{
    if (g_ui_ops.send_server_data) {
        g_ui_ops.send_server_data(g_ui_user_data, data, len);
    }
}

int powwow_has_ui_display(void)
{
    return (g_ui_ops.display_text != NULL) ? 1 : 0;
}

void powwow_internal_ui_connection_status(int connected, const char *host, int port)
{
    if (g_ui_ops.connection_status) {
        g_ui_ops.connection_status(g_ui_user_data, connected ? true : false, host, port);
    }
}

static void powwow_init_core_engine(void)
{
    if (g_initialized) return;

    group_delim = my_strdup("@");
    update_now();
    ref_time = start_time = movie_last = now;
    start_clock = cpu_clock = clock();
#ifndef NO_RANDOM
    init_random((int)now.tv_sec);
#endif

    if (!var) {
        var = (vars *)malloc(sizeof(vars) * (NUMTOT + max_named_vars));
        if (var) {
            for (int i = 0; i < NUMTOT; i++) {
                var[i].num = &global_var[i].num;
                var[i].str = &global_var[i].str;
            }
        }
    }
    paramstk.curr = 0;

    initialize_cmd();

    if (!prompt) {
        prompt = add_varnode("prompt", 1);
        if (prompt) prompt->str = ptrnew(PARAMLEN);
    }
    if (!marked_prompt) {
        marked_prompt = ptrnew(PARAMLEN);
    }
    if (!last_line) {
        last_line = add_varnode("last_line", 1);
        if (last_line) last_line->str = ptrnew(PARAMLEN);
    }
    if (!globptr[0]) {
        globptr[0] = ptrnew(PARAMLEN);
    }
    if (!globptr[1]) {
        globptr[1] = ptrnew(PARAMLEN);
    }
    g_initialized = true;
}

powwow_session_t *powwow_session_create(const powwow_ui_ops_t *ops, void *user_data)
{
    powwow_init_core_engine();

    powwow_session_t *session = (powwow_session_t *)calloc(1, sizeof(powwow_session_t));
    if (!session) return NULL;

    if (ops) {
        session->ops = *ops;
        g_ui_ops = *ops;
    }
    session->user_data = user_data;
    g_ui_user_data = user_data;
    g_current_session = session;

    return session;
}

void powwow_session_destroy(powwow_session_t *session)
{
    if (!session) return;
    if (session == g_current_session) {
        g_current_session = NULL;
        memset(&g_ui_ops, 0, sizeof(g_ui_ops));
        g_ui_user_data = NULL;
    }
    free(session);
}

int powwow_connect(powwow_session_t *session, const char *hostname, int port, bool use_tls)
{
    if (!session || !hostname) return -1;

    strncpy(session->host, hostname, sizeof(session->host) - 1);
    session->port = port;

    struct tls_options tlsopt = {0};
    tlsopt.use_tls = use_tls;

    tcp_open("main", NULL, (char *)hostname, port, &tlsopt, true);
    if (main_connection) {
        session->connected = true;
        if (session->ops.connection_status) {
            session->ops.connection_status(session->user_data, true, hostname, port);
        }
        return 0;
    }
    return -1;
}

void powwow_disconnect(powwow_session_t *session)
{
    if (!session) return;
    if (main_connection) {
        tcp_close(main_connection->id);
    }
    session->connected = false;
    if (session->ops.connection_status) {
        session->ops.connection_status(session->user_data, false, session->host, session->port);
    }
}

bool powwow_is_connected(powwow_session_t *session)
{
    if (!session) return false;
    return (main_connection != NULL);
}

void powwow_send_line(powwow_session_t *session, const char *line)
{
    if (!session || !line) return;
    if (main_connection && !current_connection) {
        current_connection = main_connection;
    }
    if (*line == '\0') {
        if (current_connection) {
            tcp_write(current_connection, "");
        } else {
            powwow_internal_ui_send_server_data("\r\n", 2);
        }
    } else {
        parse_user_input((char *)line, 1);
    }
    tcp_flush();
    tty_flush();
}

void powwow_process_remote(powwow_session_t *session, const char *buf, size_t len)
{
    (void)session;
    if (!buf || len == 0) return;
    char *copy = (char *)malloc(len + 1);
    if (!copy) return;
    memcpy(copy, buf, len);
    copy[len] = '\0';
    process_remote_input(copy, len);
    free(copy);
    if (promptlen && prompt_status == 1) {
        if (g_ui_ops.display_prompt && prompt && prompt->str) {
            powwow_internal_ui_display_prompt(ptrdata(prompt->str));
        } else if (!g_ui_ops.display_prompt && g_ui_ops.display_text && prompt && prompt->str) {
            powwow_internal_ui_display_text(ptrdata(prompt->str), strlen(ptrdata(prompt->str)));
        } else {
            draw_prompt();
        }
    }
    tty_flush();
}

void powwow_process_input(powwow_session_t *session, const char *buf, size_t len)
{
    powwow_process_remote(session, buf, len);
}

void powwow_process_network(powwow_session_t *session)
{
    (void)session;
    if (main_connection) {
        current_connection = main_connection;
        char buffer[TCP_READ_EXPANSION * BUFSIZE + 2];
        int got = tcp_read(main_connection, buffer, sizeof(buffer) - 1);
        if (got > 0) {
            buffer[got] = '\0';
            process_remote_input(buffer, got);
            if (promptlen && prompt_status == 1) {
                if (g_ui_ops.display_prompt && prompt && prompt->str) {
                    powwow_internal_ui_display_prompt(ptrdata(prompt->str));
                } else if (!g_ui_ops.display_prompt && g_ui_ops.display_text && prompt && prompt->str) {
                    powwow_internal_ui_display_text(ptrdata(prompt->str), strlen(ptrdata(prompt->str)));
                } else {
                    draw_prompt();
                }
            }
            tty_flush();
        }
    }
}

int powwow_get_connection_fd(powwow_session_t *session)
{
    (void)session;
    return main_connection ? main_connection->fd : -1;
}

void powwow_set_window_size(powwow_session_t *session, int col_count, int row_count)
{
    (void)session;
    if (col_count > 0) cols = col_count;
    if (row_count > 0) lines = row_count;
    if (main_connection) {
        tcp_write_tty_size();
    }
}

/* Inspection and mutation APIs for internal structs */

void powwow_list_aliases(powwow_session_t *session, powwow_alias_callback_t cb, void *user_data)
{
    (void)session;
    if (!cb) return;

    for (aliasnode *p = sortedaliases; p; p = p->snext) {
        powwow_alias_info_t info = {
            .name = p->name,
            .subst = p->subst,
            .group = p->group,
            .enabled = p->active != 0
        };
        cb(&info, user_data);
    }
}

int powwow_set_alias(powwow_session_t *session, const char *name, const char *subst, const char *group)
{
    (void)session;
    if (!name || !*name || !subst) return -1;

    aliasnode **np = lookup_alias((char *)name);
    if (np && *np) {
        delete_aliasnode(np);
    }
    add_aliasnode((char *)name, (char *)subst);
    if (group && *group) {
        aliasnode **p = lookup_alias((char *)name);
        if (p && *p) {
            free((*p)->group);
            (*p)->group = my_strdup(group);
        }
    }
    return 0;
}

int powwow_delete_alias(powwow_session_t *session, const char *name)
{
    (void)session;
    if (!name) return -1;
    aliasnode **np = lookup_alias((char *)name);
    if (np && *np) {
        delete_aliasnode(np);
        return 0;
    }
    return -1;
}

void powwow_list_actions(powwow_session_t *session, powwow_action_callback_t cb, void *user_data)
{
    (void)session;
    if (!cb) return;

    for (actionnode *p = actions; p; p = p->next) {
        powwow_action_info_t info = {
            .pattern = p->pattern,
            .command = p->command,
            .group = p->group,
            .enabled = p->active != 0
        };
        cb(&info, user_data);
    }
}

int powwow_set_action(powwow_session_t *session, const char *pattern, const char *cmd, const char *group)
{
    (void)session;
    if (!pattern || !*pattern || !cmd) return -1;

    actionnode **ap = lookup_action_pattern((char *)pattern);
    if (ap && *ap) {
        delete_actionnode(ap);
    }
    add_actionnode((char *)pattern, (char *)cmd, NULL, 1, 0, NULL);
    if (group && *group) {
        actionnode **p = lookup_action_pattern((char *)pattern);
        if (p && *p) {
            free((*p)->group);
            (*p)->group = my_strdup(group);
        }
    }
    return 0;
}

int powwow_delete_action(powwow_session_t *session, const char *pattern)
{
    (void)session;
    if (!pattern) return -1;
    actionnode **ap = lookup_action_pattern((char *)pattern);
    if (ap && *ap) {
        delete_actionnode(ap);
        return 0;
    }
    return -1;
}

void powwow_list_variables(powwow_session_t *session, powwow_var_callback_t cb, void *user_data)
{
    (void)session;
    if (!cb) return;

    /* Iterate through string variables (type 1) */
    for (varnode *p = sortednamed_vars[1]; p; p = p->snext) {
        if (!p->name) continue;
        powwow_var_info_t info = {
            .name = p->name,
            .value = p->str ? ptrdata(p->str) : ""
        };
        cb(&info, user_data);
    }
}

int powwow_set_variable(powwow_session_t *session, const char *name, const char *val)
{
    (void)session;
    if (!name || !*name || !val) return -1;

    varnode **vp = lookup_varnode((char *)name, 1);
    varnode *v = (vp && *vp) ? *vp : add_varnode((char *)name, 1);
    if (!v) return -1;

    v->str = ptrmcpy(v->str, (char *)val, strlen(val));
    return 0;
}

const char *powwow_get_variable(powwow_session_t *session, const char *name)
{
    (void)session;
    if (!name || !*name) return NULL;
    varnode **vp = lookup_varnode((char *)name, 1);
    if (vp && *vp && (*vp)->str) {
        return ptrdata((*vp)->str);
    }
    return NULL;
}

int powwow_delete_variable(powwow_session_t *session, const char *name)
{
    (void)session;
    if (!name || !*name) return -1;
    varnode **vp = lookup_varnode((char *)name, 1);
    if (vp && *vp) {
        delete_varnode(vp, 1);
        return 0;
    }
    return -1;
}

/* Key bindings (#bind) */
void powwow_list_binds(powwow_session_t *session, powwow_bind_callback_t cb, void *user_data)
{
    (void)session;
    if (!cb) return;

    for (keynode *p = keydefs; p; p = p->next) {
        powwow_bind_info_t info = {
            .name = p->name,
            .sequence = p->sequence,
            .command = p->call_data ? p->call_data : ""
        };
        cb(&info, user_data);
    }
}

int powwow_set_bind(powwow_session_t *session, const char *name, const char *sequence, const char *command)
{
    (void)session;
    if (!name || !*name || !sequence) return -1;

    keynode **kp = lookup_key((char *)name);
    if (kp && *kp) {
        delete_keynode(kp);
    }
    add_keynode((char *)name, (char *)sequence, strlen(sequence), key_run_command, (char *)command);
    return 0;
}

int powwow_delete_bind(powwow_session_t *session, const char *name)
{
    (void)session;
    if (!name || !*name) return -1;
    keynode **kp = lookup_key((char *)name);
    if (kp && *kp) {
        delete_keynode(kp);
        return 0;
    }
    return -1;
}

/* Markers (#mark) */
void powwow_list_marks(powwow_session_t *session, powwow_mark_callback_t cb, void *user_data)
{
    (void)session;
    if (!cb) return;

    for (marknode *p = markers; p; p = p->next) {
        powwow_mark_info_t info = {
            .pattern = p->pattern,
            .attrcode = p->attrcode,
            .at_beg = p->mbeg != 0
        };
        cb(&info, user_data);
    }
}

int powwow_set_mark(powwow_session_t *session, const char *pattern, int attrcode, bool at_beg)
{
    (void)session;
    if (!pattern || !*pattern) return -1;

    marknode **mp = lookup_marker((char *)pattern, at_beg ? 1 : 0);
    if (mp && *mp) {
        delete_marknode(mp);
    }
    add_marknode((char *)pattern, attrcode, at_beg ? 1 : 0, 0);
    return 0;
}

int powwow_delete_mark(powwow_session_t *session, const char *pattern, bool at_beg)
{
    (void)session;
    if (!pattern || !*pattern) return -1;
    marknode **mp = lookup_marker((char *)pattern, at_beg ? 1 : 0);
    if (mp && *mp) {
        delete_marknode(mp);
        return 0;
    }
    return -1;
}

/* Delays (#at / #in) */
void powwow_list_delays(powwow_session_t *session, powwow_delay_callback_t cb, void *user_data)
{
    (void)session;
    if (!cb) return;

    update_now();
    for (delaynode *p = delays; p; p = p->next) {
        powwow_delay_info_t info = {
            .name = p->name,
            .command = p->command,
            .delay_ms = diff_vtime(&p->when, &now),
            .is_active = true
        };
        cb(&info, user_data);
    }
    for (delaynode *p = dead_delays; p; p = p->next) {
        powwow_delay_info_t info = {
            .name = p->name,
            .command = p->command,
            .delay_ms = 0,
            .is_active = false
        };
        cb(&info, user_data);
    }
}

int powwow_delete_delay(powwow_session_t *session, const char *name)
{
    (void)session;
    if (!name || !*name) return -1;

    delaynode **dp = lookup_delay((char *)name, 0);
    if (dp && *dp) {
        delete_delaynode(dp);
        return 0;
    }
    dp = lookup_delay((char *)name, 1);
    if (dp && *dp) {
        delete_delaynode(dp);
        return 0;
    }
    return -1;
}

/* History (#history) */
void powwow_list_history(powwow_session_t *session, powwow_history_callback_t cb, void *user_data)
{
    (void)session;
    if (!cb) return;

    for (int i = 0; i < MAX_HIST; i++) {
        if (hist[i] && *hist[i]) {
            powwow_history_info_t info = {
                .index = i,
                .line = hist[i]
            };
            cb(&info, user_data);
        }
    }
}

void powwow_add_history(powwow_session_t *session, const char *line)
{
    (void)session;
    if (line && *line) {
        char *dup = my_strdup(line);
        if (dup) {
            put_history(dup);
            free(dup);
        }
    }
}

/* Options / Preferences */
struct option_desc {
    const char *name;
    const char *description;
    char *val_ptr;
};

static struct option_desc g_options[] = {
    { "echo", "Echo text sent to server", &opt_echo },
    { "keyecho", "Echo key bindings", &opt_keyecho },
    { "info", "Show internal informatory messages", &opt_info },
    { "history", "Save history list", &opt_history },
    { "words", "Save word completion list", &opt_words },
    { "compact", "Clear prompt between messages", &opt_compact },
    { "debug", "Echo lines before parsing", &opt_debug },
    { "speedwalk", "Enable speedwalking (n, s, e, w, etc.)", &opt_speedwalk },
    { "wrap", "Wrap words at margin", &opt_wrap },
    { "autoprint", "Print lines matched by actions", &opt_autoprint },
    { "reprint", "Reprint sent commands upon prompt", &opt_reprint },
    { "sendsize", "Send window size on connect", &opt_sendsize },
    { "autoclear", "Clear input line before running commands", &opt_autoclear },
    { "exit", "Exit when closing last connection", &opt_exit },
    { NULL, NULL, NULL }
};

void powwow_list_options(powwow_session_t *session, powwow_option_callback_t cb, void *user_data)
{
    (void)session;
    if (!cb) return;

    for (int i = 0; g_options[i].name; i++) {
        powwow_option_info_t info = {
            .name = g_options[i].name,
            .description = g_options[i].description,
            .value = (*g_options[i].val_ptr) != 0
        };
        cb(&info, user_data);
    }
}

int powwow_set_option(powwow_session_t *session, const char *name, bool value)
{
    (void)session;
    if (!name) return -1;
    for (int i = 0; g_options[i].name; i++) {
        if (strcasecmp(g_options[i].name, name) == 0) {
            *g_options[i].val_ptr = value ? 1 : 0;
            return 0;
        }
    }
    return -1;
}

int powwow_get_option(powwow_session_t *session, const char *name)
{
    (void)session;
    if (!name) return -1;
    for (int i = 0; g_options[i].name; i++) {
        if (strcasecmp(g_options[i].name, name) == 0) {
            return (*g_options[i].val_ptr) != 0 ? 1 : 0;
        }
    }
    return -1;
}

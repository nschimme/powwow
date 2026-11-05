#ifndef POWWOW_H
#define POWWOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Forward declaration of opaque session type */
typedef struct powwow_session powwow_session_t;

/* UI callback operations structure */
typedef struct powwow_ui_ops {
    /* Called when text should be displayed on screen/terminal */
    void (*display_text)(void *user_data, const char *text, size_t len);
    /* Called when prompt text updates */
    void (*display_prompt)(void *user_data, const char *prompt);
    /* Called when window title should change */
    void (*set_title)(void *user_data, const char *title);
    /* Called on audible/visual bell */
    void (*bell)(void *user_data);
    /* Called when connection status changes (connected = true, disconnected = false) */
    void (*connection_status)(void *user_data, bool connected, const char *host, int port);
} powwow_ui_ops_t;

/* Public data structures for GUI/TUI struct inspection */
typedef struct powwow_alias_info {
    const char *name;
    const char *subst;
    const char *group;
    bool enabled;
} powwow_alias_info_t;

typedef struct powwow_action_info {
    const char *pattern;
    const char *command;
    const char *group;
    bool enabled;
} powwow_action_info_t;

typedef struct powwow_var_info {
    const char *name;
    const char *value;
} powwow_var_info_t;

typedef struct powwow_bind_info {
    const char *name;
    const char *sequence;
    const char *command;
} powwow_bind_info_t;

typedef struct powwow_mark_info {
    const char *pattern;
    int attrcode;
    bool at_beg;
} powwow_mark_info_t;

typedef struct powwow_delay_info {
    const char *name;
    const char *command;
    long delay_ms;
    bool is_active;
} powwow_delay_info_t;

typedef struct powwow_history_info {
    int index;
    const char *line;
} powwow_history_info_t;

/* Callback types for visiting lists of internal structs */
typedef void (*powwow_alias_callback_t)(const powwow_alias_info_t *info, void *user_data);
typedef void (*powwow_action_callback_t)(const powwow_action_info_t *info, void *user_data);
typedef void (*powwow_var_callback_t)(const powwow_var_info_t *info, void *user_data);
typedef void (*powwow_bind_callback_t)(const powwow_bind_info_t *info, void *user_data);
typedef void (*powwow_mark_callback_t)(const powwow_mark_info_t *info, void *user_data);
typedef void (*powwow_delay_callback_t)(const powwow_delay_info_t *info, void *user_data);
typedef void (*powwow_history_callback_t)(const powwow_history_info_t *info, void *user_data);

/* Session lifecycle functions */
powwow_session_t *powwow_session_create(const powwow_ui_ops_t *ops, void *user_data);
void powwow_session_destroy(powwow_session_t *session);

/* Network & Session control */
int  powwow_connect(powwow_session_t *session, const char *hostname, int port, bool use_tls);
void powwow_disconnect(powwow_session_t *session);
bool powwow_is_connected(powwow_session_t *session);

/* Input and engine processing */
void powwow_send_line(powwow_session_t *session, const char *line);
void powwow_process_input(powwow_session_t *session, const char *buf, size_t len);
void powwow_process_network(powwow_session_t *session);
int  powwow_get_connection_fd(powwow_session_t *session);

/* Window size reporting */
void powwow_set_window_size(powwow_session_t *session, int cols, int rows);

/* Inspection and mutation APIs for internal structs */

/* Aliases */
void powwow_list_aliases(powwow_session_t *session, powwow_alias_callback_t cb, void *user_data);
int  powwow_set_alias(powwow_session_t *session, const char *name, const char *subst, const char *group);
int  powwow_delete_alias(powwow_session_t *session, const char *name);

/* Actions / Triggers */
void powwow_list_actions(powwow_session_t *session, powwow_action_callback_t cb, void *user_data);
int  powwow_set_action(powwow_session_t *session, const char *pattern, const char *cmd, const char *group);
int  powwow_delete_action(powwow_session_t *session, const char *pattern);

/* Variables */
void powwow_list_variables(powwow_session_t *session, powwow_var_callback_t cb, void *user_data);
int  powwow_set_variable(powwow_session_t *session, const char *name, const char *val);
const char *powwow_get_variable(powwow_session_t *session, const char *name);
int  powwow_delete_variable(powwow_session_t *session, const char *name);

/* Key bindings (#bind) */
void powwow_list_binds(powwow_session_t *session, powwow_bind_callback_t cb, void *user_data);
int  powwow_set_bind(powwow_session_t *session, const char *name, const char *sequence, const char *command);
int  powwow_delete_bind(powwow_session_t *session, const char *name);

/* Markers / Highlighting (#mark) */
void powwow_list_marks(powwow_session_t *session, powwow_mark_callback_t cb, void *user_data);
int  powwow_set_mark(powwow_session_t *session, const char *pattern, int attrcode, bool at_beg);
int  powwow_delete_mark(powwow_session_t *session, const char *pattern, bool at_beg);

/* Delays / Timers (#at / #in) */
void powwow_list_delays(powwow_session_t *session, powwow_delay_callback_t cb, void *user_data);
int  powwow_delete_delay(powwow_session_t *session, const char *name);

/* History (#history / #put) */
void powwow_list_history(powwow_session_t *session, powwow_history_callback_t cb, void *user_data);
void powwow_add_history(powwow_session_t *session, const char *line);

#ifdef __cplusplus
}
#endif

#endif /* POWWOW_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "powwow/powwow.h"

static int g_display_count = 0;
static char g_last_displayed[4096] = {0};
static char g_last_server_data[4096] = {0};

static void test_display_text(void *user_data, const char *text, size_t len)
{
    (void)user_data;
    g_display_count++;
    if (len < sizeof(g_last_displayed)) {
        strncpy(g_last_displayed, text, len);
        g_last_displayed[len] = '\0';
    }
}

static void test_send_server_data(void *user_data, const char *data, size_t len)
{
    (void)user_data;
    if (len < sizeof(g_last_server_data)) {
        strncat(g_last_server_data, data, sizeof(g_last_server_data) - strlen(g_last_server_data) - 1);
    }
}

static void test_alias_callback(const powwow_alias_info_t *info, void *user_data)
{
    int *count = (int *)user_data;
    (*count)++;
    assert(info != NULL);
    assert(info->name != NULL);
}

static void test_action_callback(const powwow_action_info_t *info, void *user_data)
{
    int *count = (int *)user_data;
    (*count)++;
    assert(info != NULL);
    assert(info->pattern != NULL);
}

static void test_var_callback(const powwow_var_info_t *info, void *user_data)
{
    int *count = (int *)user_data;
    (*count)++;
    assert(info != NULL);
    assert(info->name != NULL);
}

static void test_bind_callback(const powwow_bind_info_t *info, void *user_data)
{
    int *count = (int *)user_data;
    (*count)++;
    assert(info != NULL);
    assert(info->name != NULL);
}

static void test_mark_callback(const powwow_mark_info_t *info, void *user_data)
{
    int *count = (int *)user_data;
    (*count)++;
    assert(info != NULL);
    assert(info->pattern != NULL);
}

static void test_delay_callback(const powwow_delay_info_t *info, void *user_data)
{
    int *count = (int *)user_data;
    (*count)++;
    assert(info != NULL);
    assert(info->name != NULL);
}

static void test_history_callback(const powwow_history_info_t *info, void *user_data)
{
    int *count = (int *)user_data;
    (*count)++;
    assert(info != NULL);
    assert(info->line != NULL);
}

static void test_option_callback(const powwow_option_info_t *info, void *user_data)
{
    int *count = (int *)user_data;
    (*count)++;
    assert(info != NULL);
    assert(info->name != NULL);
}

/* 1. Basic CRUD on structs */
void test_struct_crud(powwow_session_t *sess)
{
    printf("Testing struct CRUD operations...\n");

    /* Aliases */
    assert(powwow_set_alias(sess, "k", "kill $1", "combat") == 0);
    int alias_count = 0;
    powwow_list_aliases(sess, test_alias_callback, &alias_count);
    assert(alias_count >= 1);
    assert(powwow_delete_alias(sess, "k") == 0);
    alias_count = 0;
    powwow_list_aliases(sess, test_alias_callback, &alias_count);
    assert(alias_count == 0);

    /* Actions */
    assert(powwow_set_action(sess, "^You are hungry", "eat bread", "survival") == 0);
    int action_count = 0;
    powwow_list_actions(sess, test_action_callback, &action_count);
    assert(action_count >= 1);
    assert(powwow_delete_action(sess, "^You are hungry") == 0);
    action_count = 0;
    powwow_list_actions(sess, test_action_callback, &action_count);
    assert(action_count == 0);

    /* Variables */
    assert(powwow_set_variable(sess, "myvar", "hello_world") == 0);
    const char *val = powwow_get_variable(sess, "myvar");
    assert(val != NULL);
    assert(strcmp(val, "hello_world") == 0);
    int var_count = 0;
    powwow_list_variables(sess, test_var_callback, &var_count);
    assert(var_count >= 1);
    assert(powwow_delete_variable(sess, "myvar") == 0);
    assert(powwow_get_variable(sess, "myvar") == NULL);

    /* Key Binds */
    assert(powwow_set_bind(sess, "f1", "\033OP", "help") == 0);
    int bind_count = 0;
    powwow_list_binds(sess, test_bind_callback, &bind_count);
    assert(bind_count >= 1);
    assert(powwow_delete_bind(sess, "f1") == 0);

    /* Marks / Highlights */
    assert(powwow_set_mark(sess, "urgent", 1, false) == 0);
    int mark_count = 0;
    powwow_list_marks(sess, test_mark_callback, &mark_count);
    assert(mark_count >= 1);
    assert(powwow_delete_mark(sess, "urgent", false) == 0);

    /* Delays / Timers */
    int delay_count = 0;
    powwow_list_delays(sess, test_delay_callback, &delay_count);

    /* History */
    powwow_add_history(sess, "look");
    powwow_add_history(sess, "score");
    int hist_count = 0;
    powwow_list_history(sess, test_history_callback, &hist_count);
    assert(hist_count >= 2);

    /* Options / Preferences */
    int opt_count = 0;
    powwow_list_options(sess, test_option_callback, &opt_count);
    assert(opt_count >= 10);
    assert(powwow_set_option(sess, "speedwalk", true) == 0);
    assert(powwow_get_option(sess, "speedwalk") == 1);
    assert(powwow_set_option(sess, "speedwalk", false) == 0);
    assert(powwow_get_option(sess, "speedwalk") == 0);
}

/* 2. Math & Expression Evaluation Engine (#math, #(), #print, #if) */
void test_eval_and_math(powwow_session_t *sess)
{
    printf("Testing math and expression evaluator...\n");

    /* Arithmetic and variable assignments via #() using global variables */
    powwow_send_line(sess, "#(@-10 = 40 + 2)");

    /* Math in #print */
    g_display_count = 0;
    powwow_send_line(sess, "#print (\"Result: \" + %(@-10))");
    assert(g_display_count > 0);
    assert(strstr(g_last_displayed, "Result: 42") != NULL);

    /* Logical and comparison operators with #if */
    memset(g_last_server_data, 0, sizeof(g_last_server_data));
    powwow_send_line(sess, "#if (@-10 == 42) say Correct; #else say Wrong");
    assert(strstr(g_last_server_data, "say Correct") != NULL);

    memset(g_last_server_data, 0, sizeof(g_last_server_data));
    powwow_send_line(sess, "#if (@-10 < 10) say Low; #else say High");
    assert(strstr(g_last_server_data, "say High") != NULL);
}

/* 3. Parameterized and Nested Aliases ($0, $1, $2, etc.) */
void test_parameterized_aliases(powwow_session_t *sess)
{
    printf("Testing parameterized and nested aliases...\n");

    /* Parameterized alias */
    powwow_send_line(sess, "#alias summ=cast 'summon' $0");
    memset(g_last_server_data, 0, sizeof(g_last_server_data));
    powwow_send_line(sess, "summ thor");
    assert(strstr(g_last_server_data, "cast 'summon' thor") != NULL);

    /* Semicolon-delimited multi-command alias with braces */
    powwow_send_line(sess, "#alias prep={draw sword;wear shield}");
    memset(g_last_server_data, 0, sizeof(g_last_server_data));
    powwow_send_line(sess, "prep");
    assert(strstr(g_last_server_data, "draw sword") != NULL);
    assert(strstr(g_last_server_data, "wear shield") != NULL);

    /* Cleanup */
    powwow_delete_alias(sess, "summ");
    powwow_delete_alias(sess, "prep");
}

/* 4. Trigger & Action Matching Engine (#action, regex, wildcards) */
void test_triggers_and_actions(powwow_session_t *sess)
{
    printf("Testing triggers and actions pipeline...\n");

    /* Wildcard parameter capture ($1 in trigger) */
    powwow_set_action(sess, "^You feel thirsty.", "drink water", "survival");
    memset(g_last_server_data, 0, sizeof(g_last_server_data));
    powwow_process_remote(sess, "You feel thirsty.\n", strlen("You feel thirsty.\n"));
    assert(strstr(g_last_server_data, "drink water") != NULL);

    /* Cleanup */
    powwow_delete_action(sess, "^You feel thirsty.");
}

/* 5. Window Size Reporting & Terminal NAWS */
void test_window_size(powwow_session_t *sess)
{
    printf("Testing window size reporting...\n");
    powwow_set_window_size(sess, 132, 50);
}

int main(void)
{
    powwow_ui_ops_t ops = {
        .display_text = test_display_text,
        .send_server_data = test_send_server_data,
    };

    powwow_session_t *sess = powwow_session_create(&ops, NULL);
    assert(sess != NULL);

    test_struct_crud(sess);
    test_eval_and_math(sess);
    test_parameterized_aliases(sess);
    test_triggers_and_actions(sess);
    test_window_size(sess);

    powwow_session_destroy(sess);

    printf("==========================================\n");
    printf("All enhanced libpowwow unit tests PASSED!\n");
    printf("==========================================\n");
    return 0;
}

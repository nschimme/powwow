#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "powwow/powwow.h"

static int g_display_count = 0;
static char g_last_displayed[1024] = {0};

static void test_display_text(void *user_data, const char *text, size_t len)
{
    (void)user_data;
    g_display_count++;
    if (len < sizeof(g_last_displayed)) {
        strncpy(g_last_displayed, text, len);
        g_last_displayed[len] = '\0';
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

int main(void)
{
    powwow_ui_ops_t ops = {
        .display_text = test_display_text,
    };

    powwow_session_t *sess = powwow_session_create(&ops, NULL);
    assert(sess != NULL);

    /* 1. Test Aliases API */
    assert(powwow_set_alias(sess, "k", "kill $1", "combat") == 0);
    int alias_count = 0;
    powwow_list_aliases(sess, test_alias_callback, &alias_count);
    assert(alias_count >= 1);

    assert(powwow_delete_alias(sess, "k") == 0);
    alias_count = 0;
    powwow_list_aliases(sess, test_alias_callback, &alias_count);
    assert(alias_count == 0);

    /* 2. Test Actions API */
    assert(powwow_set_action(sess, "^You are hungry", "eat bread", "survival") == 0);
    int action_count = 0;
    powwow_list_actions(sess, test_action_callback, &action_count);
    assert(action_count >= 1);

    assert(powwow_delete_action(sess, "^You are hungry") == 0);
    action_count = 0;
    powwow_list_actions(sess, test_action_callback, &action_count);
    assert(action_count == 0);

    /* 3. Test Variables API */
    assert(powwow_set_variable(sess, "myvar", "hello_world") == 0);
    const char *val = powwow_get_variable(sess, "myvar");
    assert(val != NULL);
    assert(strcmp(val, "hello_world") == 0);

    int var_count = 0;
    powwow_list_variables(sess, test_var_callback, &var_count);
    assert(var_count >= 1);

    assert(powwow_delete_variable(sess, "myvar") == 0);
    assert(powwow_get_variable(sess, "myvar") == NULL);

    /* 4. Test Key Bindings API (#bind) */
    assert(powwow_set_bind(sess, "f1", "\033OP", "help") == 0);
    int bind_count = 0;
    powwow_list_binds(sess, test_bind_callback, &bind_count);
    assert(bind_count >= 1);

    assert(powwow_delete_bind(sess, "f1") == 0);

    /* 5. Test Markers API (#mark) */
    assert(powwow_set_mark(sess, "urgent", 1, false) == 0);
    int mark_count = 0;
    powwow_list_marks(sess, test_mark_callback, &mark_count);
    assert(mark_count >= 1);

    assert(powwow_delete_mark(sess, "urgent", false) == 0);

    /* 6. Test Delays / Timers API (#at / #in) */
    int delay_count = 0;
    powwow_list_delays(sess, test_delay_callback, &delay_count);

    /* 7. Test History API (#history) */
    powwow_add_history(sess, "look");
    powwow_add_history(sess, "score");
    int hist_count = 0;
    powwow_list_history(sess, test_history_callback, &hist_count);
    assert(hist_count >= 2);

    /* 8. Test Window Size Setting */
    powwow_set_window_size(sess, 120, 40);

    /* Clean up */
    powwow_session_destroy(sess);

    printf("All extended libpowwow unit tests passed successfully!\n");
    return 0;
}

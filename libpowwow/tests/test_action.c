#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "powwow/powwow.h"

static char g_last_server[1024] = {0};
static void send_cb(void *u, const char *s, size_t l) {
    (void)u;
    if (l < sizeof(g_last_server)) {
        strncat(g_last_server, s, sizeof(g_last_server) - strlen(g_last_server) - 1);
    }
}

static void action_cb(const powwow_action_info_t *info, void *u) {
    int *c = (int *)u;
    (*c)++;
    assert(info->pattern != NULL);
    assert(info->command != NULL);
}

int main(void) {
    powwow_ui_ops_t ops = { .send_server_data = send_cb };
    powwow_session_t *s = powwow_session_create(&ops, NULL);
    assert(s != NULL);

    /* 1. Add, list, delete */
    assert(powwow_set_action(s, "^You are thirsty.", "drink water", "survival") == 0);
    int count = 0;
    powwow_list_actions(s, action_cb, &count);
    assert(count == 1);

    /* 2. Trigger on remote input */
    memset(g_last_server, 0, sizeof(g_last_server));
    powwow_process_remote(s, "You are thirsty.\n", strlen("You are thirsty.\n"));
    assert(strstr(g_last_server, "drink water") != NULL);

    assert(powwow_delete_action(s, "^You are thirsty.") == 0);
    count = 0;
    powwow_list_actions(s, action_cb, &count);
    assert(count == 0);

    /* 3. Non-matching input */
    memset(g_last_server, 0, sizeof(g_last_server));
    powwow_process_remote(s, "Something unrelated happens.\n", strlen("Something unrelated happens.\n"));
    assert(strlen(g_last_server) == 0);

    powwow_session_destroy(s);
    printf("test_action PASSED!\n");
    return 0;
}

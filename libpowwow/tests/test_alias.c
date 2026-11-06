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

static void alias_cb(const powwow_alias_info_t *info, void *u) {
    int *c = (int *)u;
    (*c)++;
    assert(info->name != NULL);
    assert(info->subst != NULL);
}

int main(void) {
    powwow_ui_ops_t ops = { .send_server_data = send_cb };
    powwow_session_t *s = powwow_session_create(&ops, NULL);
    assert(s != NULL);

    /* 1. Add, list, delete */
    assert(powwow_set_alias(s, "k", "kill rat", "combat") == 0);
    int count = 0;
    powwow_list_aliases(s, alias_cb, &count);
    assert(count == 1);

    memset(g_last_server, 0, sizeof(g_last_server));
    powwow_send_line(s, "k");
    assert(strstr(g_last_server, "kill rat") != NULL);

    assert(powwow_delete_alias(s, "k") == 0);
    count = 0;
    powwow_list_aliases(s, alias_cb, &count);
    assert(count == 0);

    /* 2. Parameterized aliases */
    powwow_send_line(s, "#alias summ=cast 'summon' $0");
    memset(g_last_server, 0, sizeof(g_last_server));
    powwow_send_line(s, "summ thor");
    assert(strstr(g_last_server, "cast 'summon' thor") != NULL);
    powwow_delete_alias(s, "summ");

    /* 3. Nested multi-command aliases */
    powwow_send_line(s, "#alias prep={draw sword;wear shield}");
    memset(g_last_server, 0, sizeof(g_last_server));
    powwow_send_line(s, "prep");
    assert(strstr(g_last_server, "draw sword") != NULL);
    assert(strstr(g_last_server, "wear shield") != NULL);
    powwow_delete_alias(s, "prep");

    powwow_session_destroy(s);
    printf("test_alias PASSED!\n");
    return 0;
}

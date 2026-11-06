#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "powwow/powwow.h"

static void dummy_display(void *u, const char *s, size_t l) { (void)u; (void)s; (void)l; }

static void opt_cb(const powwow_option_info_t *info, void *u) {
    int *c = (int *)u;
    (*c)++;
    assert(info != NULL);
    assert(info->name != NULL);
    assert(info->description != NULL);
}

int main(void) {
    powwow_ui_ops_t ops = { .display_text = dummy_display };

    /* 1. Create and destroy session */
    powwow_session_t *s = powwow_session_create(&ops, NULL);
    assert(s != NULL);
    assert(!powwow_is_connected(s));

    /* 2. Window resizing */
    powwow_set_window_size(s, 100, 30);

    /* 3. Options listing and manipulation */
    int count = 0;
    powwow_list_options(s, opt_cb, &count);
    assert(count >= 10);

    assert(powwow_set_option(s, "echo", false) == 0);
    assert(powwow_get_option(s, "echo") == 0);
    assert(powwow_set_option(s, "echo", true) == 0);
    assert(powwow_get_option(s, "echo") == 1);

    assert(powwow_set_option(s, "nonexistent_option", true) == -1);
    assert(powwow_get_option(s, "nonexistent_option") == -1);

    powwow_session_destroy(s);

    /* 4. Multiple independent sessions */
    powwow_session_t *s1 = powwow_session_create(&ops, NULL);
    powwow_session_t *s2 = powwow_session_create(&ops, NULL);
    assert(s1 != NULL);
    assert(s2 != NULL);

    powwow_session_destroy(s1);
    powwow_session_destroy(s2);

    printf("test_session PASSED!\n");
    return 0;
}

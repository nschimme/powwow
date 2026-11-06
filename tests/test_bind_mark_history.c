#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "powwow/powwow.h"

static void bind_cb(const powwow_bind_info_t *info, void *u) {
    int *c = (int *)u;
    (*c)++;
    assert(info->name != NULL);
}

static void mark_cb(const powwow_mark_info_t *info, void *u) {
    int *c = (int *)u;
    (*c)++;
    assert(info->pattern != NULL);
}

static void hist_cb(const powwow_history_info_t *info, void *u) {
    int *c = (int *)u;
    (*c)++;
    assert(info->line != NULL);
}

int main(void) {
    powwow_session_t *s = powwow_session_create(NULL, NULL);
    assert(s != NULL);

    /* 1. Key binds */
    assert(powwow_set_bind(s, "f2", "\033OQ", "score") == 0);
    int b_count = 0;
    powwow_list_binds(s, bind_cb, &b_count);
    assert(b_count >= 1);
    assert(powwow_delete_bind(s, "f2") == 0);

    /* 2. Marks */
    assert(powwow_set_mark(s, "critical", 1, true) == 0);
    int m_count = 0;
    powwow_list_marks(s, mark_cb, &m_count);
    assert(m_count >= 1);
    assert(powwow_delete_mark(s, "critical", true) == 0);

    /* 3. History */
    powwow_add_history(s, "cast heal");
    powwow_add_history(s, "rest");
    int h_count = 0;
    powwow_list_history(s, hist_cb, &h_count);
    assert(h_count >= 2);

    powwow_session_destroy(s);
    printf("test_bind_mark_history PASSED!\n");
    return 0;
}

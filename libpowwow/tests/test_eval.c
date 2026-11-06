#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "powwow/powwow.h"

static int g_display_count = 0;
static char g_last_displayed[1024] = {0};

static void test_display_text(void *u, const char *s, size_t l) {
    (void)u;
    g_display_count++;
    if (l < sizeof(g_last_displayed)) {
        strncpy(g_last_displayed, s, l);
        g_last_displayed[l] = '\0';
    }
}

int main(void) {
    powwow_ui_ops_t ops = { .display_text = test_display_text };
    powwow_session_t *s = powwow_session_create(&ops, NULL);
    assert(s != NULL);

    /* 1. Global numeric variables and arithmetic */
    powwow_send_line(s, "#(@-10 = 100 + 25)");
    g_display_count = 0;
    powwow_send_line(s, "#print (\"Sum: \" + %(@-10))");
    assert(strstr(g_last_displayed, "Sum: 125") != NULL);

    /* 2. String variables and string length */
    powwow_send_line(s, "#($greeting = \"Hello Powwow\")");
    powwow_send_line(s, "#(@-11 = .?$greeting)");
    g_display_count = 0;
    powwow_send_line(s, "#print (\"Length: \" + %(@-11))");
    assert(strstr(g_last_displayed, "Length: 12") != NULL);

    /* 3. Bitwise operators */
    powwow_send_line(s, "#(@-12 = #F & #6)");
    g_display_count = 0;
    powwow_send_line(s, "#print (\"Bitwise: \" + %(@-12))");
    assert(strstr(g_last_displayed, "Bitwise: 6") != NULL);

    /* 4. Public variable API */
    assert(powwow_set_variable(s, "hero", "Aragorn") == 0);
    const char *v = powwow_get_variable(s, "hero");
    assert(v != NULL && strcmp(v, "Aragorn") == 0);
    assert(powwow_delete_variable(s, "hero") == 0);
    assert(powwow_get_variable(s, "hero") == NULL);

    powwow_session_destroy(s);
    printf("test_eval PASSED!\n");
    return 0;
}

#include "defines.h"
#include "ui_hook.h"
#include "main.h"
#include "edit.h"
#include "tty.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Fallback stub implementations when frontend does not link tty/edit */
__attribute__((weak)) void clear_input_line(int deleteprompt) { (void)deleteprompt; }
__attribute__((weak)) void draw_input_line(void) {}
__attribute__((weak)) void draw_prompt(void) {}
__attribute__((weak)) void to_input_line(char *str) { (void)str; }
__attribute__((weak)) void key_run_command(char *cmd) { (void)cmd; }
__attribute__((weak)) void put_history(char *str) { (void)str; }
__attribute__((weak)) void fmt_next_edit(const char *fmt, ...) { (void)fmt; }
__attribute__((weak)) void put_word(char *s) { (void)s; }
__attribute__((weak)) void set_custom_delimeters(char *s) { (void)s; }

__attribute__((weak)) void tty_puts(const char *s) { if (s) powwow_internal_ui_display_text(s, strlen(s)); }
__attribute__((weak)) void tty_putc(char c) { char b[2] = {c, 0}; powwow_internal_ui_display_text(b, 1); }
__attribute__((weak)) int  tty_printf(const char *format, ...) {
    char buf[4096];
    va_list ap;
    va_start(ap, format);
    int n = vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    if (n > 0) {
        powwow_internal_ui_display_text(buf, n < (int)sizeof(buf) ? n : sizeof(buf) - 1);
    }
    return n;
}
__attribute__((weak)) void tty_flush(void) {}
__attribute__((weak)) void tty_raw_write(char *data, size_t len) { if (data && len) powwow_internal_ui_display_text(data, len); }
__attribute__((weak)) int  tty_cell_fits(int col, const char *s, size_t n, int w) { (void)col; (void)s; (void)n; (void)w; return 1; }
__attribute__((weak)) void tty_special_keys(void) {}
__attribute__((weak)) void tty_sig_winch_bottomhalf(void) {}
__attribute__((weak)) void tty_add_walk_binds(void) {}
__attribute__((weak)) void tty_add_initial_binds(void) {}
__attribute__((weak)) void tty_quit(void) {}
__attribute__((weak)) void tty_start(void) {}

__attribute__((weak)) char *tty_clreoln = "";
__attribute__((weak)) char *tty_clreoscr = "";
__attribute__((weak)) char *tty_begoln = "\r";
__attribute__((weak)) char *tty_modebold = "";
__attribute__((weak)) char *tty_modeblink = "";
__attribute__((weak)) char *tty_modeuline = "";
__attribute__((weak)) char *tty_modenorm = "";
__attribute__((weak)) char *tty_modenormbackup = "";
__attribute__((weak)) char *tty_modeinv = "";
__attribute__((weak)) char *tty_modestandon = "";
__attribute__((weak)) char *tty_modestandoff = "";

__attribute__((weak)) edit_function internal_functions[] = {
    {(char *)0, (function_str)0 }
};
__attribute__((weak)) int lookup_edit_name(char *name, char **arg) { (void)name; if (arg) *arg = NULL; return 0; }
__attribute__((weak)) int lookup_edit_function(function_str funct) { (void)funct; return 0; }

__attribute__((weak)) char *hist[MAX_HIST] = {0};
__attribute__((weak)) int curline = 0;
__attribute__((weak)) int pickline = 0;
__attribute__((weak)) wordnode words[MAX_WORDS] = {{0}};
__attribute__((weak)) int wordindex = 0;

__attribute__((weak)) int tty_read_fd = -1;
__attribute__((weak)) void tty_gotoxy(int col, int line) { (void)col; (void)line; }
__attribute__((weak)) void put_static_word(char *s) { (void)s; }
__attribute__((weak)) int  tty_has_chars(void) { return 0; }
__attribute__((weak)) int  tty_read(char *buf, size_t count) { (void)buf; (void)count; return 0; }
__attribute__((weak)) void putbackcursor(void) {}
__attribute__((weak)) void insert_char(char c) { (void)c; }
__attribute__((weak)) void insert_flush(void) {}

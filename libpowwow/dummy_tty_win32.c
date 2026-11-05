#ifdef _WIN32

#include "defines.h"
#include "tty.h"
#include <stdio.h>
#include <stdlib.h>

int tty_read_fd = 0;
char *tty_clreoln = "";
char *tty_clreoscr = "";
char *tty_begoln = "";
char *tty_modebold = "";
char *tty_modeblink = "";
char *tty_modeuline = "";
char *tty_modenorm = "";
char *tty_modenormbackup = "";
char *tty_modeinv = "";
char *tty_modestandon = "";
char *tty_modestandoff = "";

void tty_bootstrap(void) {}
void tty_start(void) {}
void tty_quit(void) {}
void tty_special_keys(void) {}
void tty_sig_winch_bottomhalf(void) {}
void tty_add_walk_binds(void) {}
void tty_add_initial_binds(void) {}
void tty_gotoxy(int col, int line) { (void)col; (void)line; }
void tty_gotoxy_opt(int fromcol, int fromline, int tocol, int toline) {
    (void)fromcol; (void)fromline; (void)tocol; (void)toline;
}
int  tty_cell_fits(int col, const char *s, size_t n, int w) {
    (void)col; (void)s; (void)n; (void)w;
    return 1;
}
void tty_puts(const char *s) {
    if (s) {
        powwow_internal_ui_display_text(s, strlen(s));
    }
}
void tty_putc(char c) {
    char buf[2] = {c, 0};
    powwow_internal_ui_display_text(buf, 1);
}
int  tty_printf(const char *format, ...) {
    char buf[1024];
    va_list va;
    va_start(va, format);
    int res = vsnprintf(buf, sizeof(buf), format, va);
    va_end(va);
    if (res > 0) {
        powwow_internal_ui_display_text(buf, res);
    }
    return res;
}
int  tty_read(char *buf, size_t count) { (void)buf; (void)count; return 0; }
void tty_gets(char *s, int size) { (void)s; (void)size; }
void tty_flush(void) {}
void tty_raw_write(char *data, size_t len) { (void)data; (void)len; }
int  tty_has_chars(void) { return 0; }

#endif

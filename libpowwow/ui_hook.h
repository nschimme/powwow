#ifndef POWWOW_UI_HOOK_H
#define POWWOW_UI_HOOK_H

#include <stddef.h>

void powwow_internal_ui_display_text(const char *text, size_t len);
void powwow_internal_ui_display_prompt(const char *prompt);
void powwow_internal_ui_set_title(const char *title);
void powwow_internal_ui_bell(void);

#endif /* POWWOW_UI_HOOK_H */

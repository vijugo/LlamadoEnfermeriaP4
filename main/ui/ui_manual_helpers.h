#ifndef UI_MANUAL_HELPERS_H
#define UI_MANUAL_HELPERS_H

#include <stdbool.h>

// Declarations of UI helper stubs for existing code to compile with SquareLine
// UI.

void ui_set_wifi_status(bool connected, const char *ssid);
void ui_update_espera_data(bool wifi, bool sd);
void ui_show_screen_arranque(const char *version, const char *fecha);
void ui_show_screen_espera(void);
void ui_show_screen_para(void);

#endif

#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"
#include <stdbool.h>

// WiFi Configuration
#define WIFI_SSID "FGonzalez3_2.4ETB"
#define WIFI_PASS "Familia123"

void wifi_init_sta(void);
bool wifi_is_connected(void);

#endif
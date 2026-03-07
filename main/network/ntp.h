#ifndef NTP_H
#define NTP_H

#include "esp_err.h"

/**
 * @brief Initialize NTP synchronization
 *
 */
void ntp_init(void);

/**
 * @brief Start time synchronization (called after WiFi is connected)
 *
 */
void ntp_sync_time(void);

#endif // NTP_H

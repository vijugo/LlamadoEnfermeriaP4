#ifndef RTC_H
#define RTC_H

#include "esp_err.h"
#include <time.h>

// RTC Pins (Based on P4 Schematic)
#define RTC_I2C_PORT 1
#define RTC_PIN_SDA 7
#define RTC_PIN_SCL 8
#define RTC_ADDR 0x32

/**
 * @brief Initialize the RTC hardware
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rtc_init(void);

/**
 * @brief Set the RTC time
 *
 * @param tm Pointer to struct tm with the time to set
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rtc_set_time(const struct tm *tm);

/**
 * @brief Get the RTC time
 *
 * @param tm Pointer to struct tm to store the current time
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rtc_get_time(struct tm *tm);

#endif // RTC_H

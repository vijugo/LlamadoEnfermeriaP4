#ifndef SD_CARD_H
#define SD_CARD_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the SD card using SDMMC
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sd_card_init(void);
bool sd_card_is_mounted(void);

#ifdef __cplusplus
}
#endif

#endif // SD_CARD_H

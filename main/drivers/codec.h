#ifndef CODEC_H
#define CODEC_H

#include "esp_codec_dev.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Pins (Confirmed for ESP32-P4 JC8012)
#define CODEC_I2S_SCLK 12
#define CODEC_I2S_MCLK 13
#define CODEC_I2S_LCLK 10
#define CODEC_I2S_DOUT 9
#define CODEC_I2S_DSIN 11
#define CODEC_PA_ENABLE_GPIO 20

esp_err_t codec_init(void);
esp_err_t codec_set_volume(int volume);
esp_err_t codec_play_audio_file(int file_number, int volume, int repeats);

#ifdef __cplusplus
}
#endif

#endif

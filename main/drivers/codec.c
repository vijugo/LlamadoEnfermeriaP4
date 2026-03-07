#include "codec.h"
#include "esp_log.h"

static const char *TAG = "codec";

// STUB: Audio desactivado para priorizar Video/Imagen
esp_err_t codec_init(void) {
  ESP_LOGW(TAG, "Audio Subsystem DISABLED by User Request.");
  return ESP_OK; // Return OK to allow boot to continue
}

esp_err_t codec_play_audio_file(int file_number, int volume, int repeats) {
  return ESP_OK;
}

esp_err_t codec_set_volume(int volume) { return ESP_OK; }

#include "psram_images.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <stdio.h>

const lv_image_dsc_t *img_espera = NULL;
const lv_image_dsc_t *img_plantilla_01 = NULL;
const lv_image_dsc_t *img_f_configura = NULL;
const lv_image_dsc_t *img_fondohospital = NULL;
const lv_image_dsc_t *img_b_configuracion = NULL;
const lv_image_dsc_t *img_b_historial = NULL;
const lv_image_dsc_t *img_b_llamado = NULL;
const lv_image_dsc_t *img_botongrandeerror = NULL;
const lv_image_dsc_t *img_botongrandeok = NULL;
const lv_image_dsc_t *img_gear = NULL;

static lv_image_dsc_t *load_img_to_psram(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    ESP_LOGE("PSRAM_IMG", "Failed to open %s", path);
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (size <= 12) {
    fclose(f);
    return NULL;
  }

  uint8_t header[12];
  if (fread(header, 1, 12, f) != 12) {
    fclose(f);
    return NULL;
  }

  long data_size = size - 12;
  // Allocate the pixel data in SPIRAM MUST BE 64-byte aligned for PPA
  uint8_t *data_buf = heap_caps_aligned_alloc(64, data_size, MALLOC_CAP_SPIRAM);
  if (!data_buf) {
    ESP_LOGE("PSRAM_IMG", "OOM loading %s (%ld bytes)", path, data_size);
    fclose(f);
    return NULL;
  }

  if (fread(data_buf, 1, data_size, f) != data_size) {
    ESP_LOGE("PSRAM_IMG", "Read error %s", path);
    heap_caps_free(data_buf);
    fclose(f);
    return NULL;
  }
  fclose(f);

  lv_image_dsc_t *dsc = heap_caps_calloc(1, sizeof(lv_image_dsc_t),
                                         MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!dsc) {
    heap_caps_free(data_buf);
    return NULL;
  }

  dsc->header.magic = header[0];
  dsc->header.cf = header[1];
  dsc->header.w = header[4] | (header[5] << 8);
  dsc->header.h = header[6] | (header[7] << 8);
  dsc->header.stride = header[8] | (header[9] << 8);
  dsc->data_size = data_size;
  dsc->data = data_buf;

  ESP_LOGI("PSRAM_IMG", "Loaded %s successfully to PSRAM (CF: 0x%02X, %dx%d)",
           path, dsc->header.cf, dsc->header.w, dsc->header.h);
  return dsc;
}

void init_psram_images(void) {
  img_espera = load_img_to_psram("/spiffs/espera.bin");
  img_plantilla_01 = load_img_to_psram("/spiffs/plantilla_01.bin");
  img_f_configura = load_img_to_psram("/spiffs/f_configura.bin");
  img_fondohospital = load_img_to_psram("/spiffs/fondohospital.bin");
  img_b_configuracion = load_img_to_psram("/spiffs/b_configuracion.bin");
  img_b_historial = load_img_to_psram("/spiffs/b_historial.bin");
  img_b_llamado = load_img_to_psram("/spiffs/b_llamado.bin");
  img_botongrandeerror = load_img_to_psram("/spiffs/botongrandeerror.bin");
  img_botongrandeok = load_img_to_psram("/spiffs/botongrandeok.bin");
  img_gear = load_img_to_psram("/spiffs/gear_8248448.bin");
}

#include "Aplicacion.h"
#include "drivers/codec.h"
#include "drivers/display.h"
#include "drivers/fast_task.h"
#include "drivers/rtc.h"
#include "drivers/sd_card.h"
#include "drivers/touch.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_spiffs.h"
#include "led_strip.h"
#include "lvgl.h"
#include "network/ntp.h"
#include "network/wifi.h"
#include "nvs_flash.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include "ui/psram_images.h"
#include "ui/ui.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *TAG = "main";
static led_strip_handle_t led_strip;

void set_led_color(uint8_t r, uint8_t g, uint8_t b) {
  if (!led_strip)
    return;
  led_strip_set_pixel(led_strip, 0, r, g, b);
  led_strip_refresh(led_strip);
}

void init_led(void) {
  led_strip_config_t strip_config = {.strip_gpio_num = 25, .max_leds = 1};
  led_strip_rmt_config_t rmt_config = {.resolution_hz = 10 * 1000 * 1000};
  ESP_ERROR_CHECK(
      led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
}

// --- LVGL FILE SYSTEM DRIVERS ---

static void *lv_sd_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode) {
  char full_path[128];
  snprintf(full_path, sizeof(full_path), "/sd/%s", path);
  const char *flags = "";
  if (mode == LV_FS_MODE_WR)
    flags = "wb";
  else if (mode == LV_FS_MODE_RD)
    flags = "rb";
  else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD))
    flags = "rb+";
  return fopen(full_path, flags);
}

static void *lv_spiffs_open(lv_fs_drv_t *drv, const char *path,
                            lv_fs_mode_t mode) {
  char full_path[128];
  if (path[0] == '/') {
    snprintf(full_path, sizeof(full_path), "/spiffs%s", path);
  } else {
    snprintf(full_path, sizeof(full_path), "/spiffs/%s", path);
  }

  const char *flags = "";
  if (mode == LV_FS_MODE_WR)
    flags = "wb";
  else if (mode == LV_FS_MODE_RD)
    flags = "rb";
  else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD))
    flags = "rb+";
  return fopen(full_path, flags);
}

static lv_fs_res_t lv_sd_close(lv_fs_drv_t *drv, void *file_p) {
  fclose(file_p);
  return LV_FS_RES_OK;
}

static lv_fs_res_t lv_sd_read(lv_fs_drv_t *drv, void *file_p, void *buf,
                              uint32_t btr, uint32_t *br) {
  *br = fread(buf, 1, btr, file_p);
  return LV_FS_RES_OK;
}

static lv_fs_res_t lv_sd_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos,
                              lv_fs_whence_t whence) {
  int w = SEEK_SET;
  if (whence == LV_FS_SEEK_END)
    w = SEEK_END;
  else if (whence == LV_FS_SEEK_CUR)
    w = SEEK_CUR;
  fseek(file_p, pos, w);
  return LV_FS_RES_OK;
}

static lv_fs_res_t lv_sd_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p) {
  *pos_p = ftell(file_p);
  return LV_FS_RES_OK;
}

static void lv_fs_setup(void) {
  // SD Card Driver (S:)
  static lv_fs_drv_t sd_drv;
  lv_fs_drv_init(&sd_drv);
  sd_drv.letter = 'S';
  sd_drv.open_cb = lv_sd_open;
  sd_drv.close_cb = lv_sd_close;
  sd_drv.read_cb = lv_sd_read;
  sd_drv.seek_cb = lv_sd_seek;
  sd_drv.tell_cb = lv_sd_tell;
  lv_fs_drv_register(&sd_drv);
  ESP_LOGI(TAG, "LVGL FS Driver 'S:' registered mapped to /sd");

  // SPIFFS Driver (A: for Assets)
  static lv_fs_drv_t spiffs_drv;
  lv_fs_drv_init(&spiffs_drv);
  spiffs_drv.letter = 'A';
  spiffs_drv.open_cb = lv_spiffs_open;
  spiffs_drv.close_cb = lv_sd_close; // Reusing generic FILE* callbacks
  spiffs_drv.read_cb = lv_sd_read;
  spiffs_drv.seek_cb = lv_sd_seek;
  spiffs_drv.tell_cb = lv_sd_tell;
  lv_fs_drv_register(&spiffs_drv);
  ESP_LOGI(TAG, "LVGL FS Driver 'A:' registered mapped to /spiffs");
}

void app_main(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGE("VERIFY", "!!! SYSTEM START V5.5 (ASSETS IN SPIFFS) !!!");
  init_led();
  set_led_color(0, 0, 255);

  // 1. MONTAR SPIFFS ANTES QUE NADA
  ESP_LOGI(TAG, "Montando partición SPIFFS...");
  esp_vfs_spiffs_conf_t spiffs_conf = {.base_path = "/spiffs",
                                       .partition_label = "storage",
                                       .max_files = 10,
                                       .format_if_mount_failed = true};
  esp_err_t sp_ret = esp_vfs_spiffs_register(&spiffs_conf);
  if (sp_ret != ESP_OK) {
    ESP_LOGE(TAG, "FALLO AL MONTAR SPIFFS: %s", esp_err_to_name(sp_ret));
  } else {
    size_t total = 0, used = 0;
    esp_spiffs_info("storage", &total, &used);
    ESP_LOGI(TAG, "SPIFFS OK. Total: %d, Usado: %d", total, used);
  }

  // 2. INICIALIZAR LVGL PORT
  lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  lvgl_cfg.task_stack = 64 * 1024;
  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

  // 3. HARDWARE ADICIONAL
  sd_card_init();

  if (lvgl_port_lock(-1)) {
    lv_fs_setup(); // Registrar drivers A: y S:

    display_init();
    touch_init();
    rtc_init();
    codec_init();

    init_psram_images();

    ui_init();
    lv_image_cache_resize(8 * 1024 * 1024, true);

    aplicacion_init(); // Iniciar lógica principal

    lvgl_port_unlock();
  }

  ESP_LOGI(TAG, "System Running");
  wifi_init_sta();
  ntp_init();
  fast_task_init();
}

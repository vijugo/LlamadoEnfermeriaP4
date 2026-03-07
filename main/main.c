#include "Aplicacion.h"
#include "drivers/codec.h"
#include "drivers/display.h"
#include "drivers/fast_task.h"
#include "drivers/rtc.h"
#include "drivers/sd_card.h"
#include "drivers/touch.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "led_strip.h"
#include "lvgl.h"
#include "network/ntp.h"
#include "network/wifi.h"
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

// --- LVGL FILE SYSTEM DRIVER (S: -> /sd) ---

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
  static lv_fs_drv_t drv;
  lv_fs_drv_init(&drv);
  drv.letter = 'S'; // Drive letter: S
  drv.open_cb = lv_sd_open;
  drv.close_cb = lv_sd_close;
  drv.read_cb = lv_sd_read;
  drv.seek_cb = lv_sd_seek;
  drv.tell_cb = lv_sd_tell;
  lv_fs_drv_register(&drv);
  ESP_LOGI(TAG, "LVGL FS Driver 'S:' registered mapped to /sd");
}

void app_main(void) {
  ESP_LOGE("VERIFY", "!!! SYSTEM START V5.4 (FS DRIVER) !!!");
  init_led();
  set_led_color(0, 0, 255);

  lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  lvgl_cfg.task_stack = 64 * 1024; // 64KB Stack for LVGL Task
  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

  sd_card_init();

  if (lvgl_port_lock(-1)) {
    // REGISTER FILE SYSTEM DRIVER!!!
    lv_fs_setup();

    display_init();
    touch_init();
    rtc_init();
    codec_init();
    ui_init();
    aplicacion_init();

    // Print RTC Time
    struct tm now;
    if (rtc_get_time(&now) == ESP_OK) {
      ESP_LOGI(TAG, "Current RTC Time: %04d-%02d-%02d %02d:%02d:%02d",
               now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour,
               now.tm_min, now.tm_sec);
    }

    // Test File Access
    FILE *f = fopen("/sd/test.jpg", "rb");
    if (f) {
      ESP_LOGI(TAG, "File /sd/test.jpg FOUND!");
      fclose(f);
      set_led_color(0, 255, 0);
    } else {
      ESP_LOGE(TAG, "File /sd/test.jpg NOT FOUND!");
      set_led_color(255, 0, 0);
    }

    // ui_init(); // Removed duplicate call
    // aplicacion_init(); // Moved after ui_init (already called above)
    lvgl_port_unlock();
  }

  ESP_LOGI(TAG, "System Running");
  wifi_init_sta();
  ntp_init();
  fast_task_init();
}

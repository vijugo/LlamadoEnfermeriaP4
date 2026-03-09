// display.c - driver for JD9365 panel
#include "display.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_jd9365.h"
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
static const char *TAG = "display";

static lv_display_t *g_lv_display = NULL;
// ... (rest of file unchanged) ...

static esp_lcd_panel_handle_t panel_handle = NULL;
static uint8_t *g_rotated_buf = NULL;

#include "esp_cache.h"
// Custom Flush Callback for LVGL
static void lvgl_dsi_flush(lv_display_t *disp, const lv_area_t *area,
                           uint8_t *px_map) {
  esp_lcd_panel_handle_t panel =
      (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);

  lv_display_rotation_t rotation = lv_display_get_rotation(disp);
  lv_area_t rotated_area = *area;

  if (rotation != LV_DISPLAY_ROTATION_0 && g_rotated_buf != NULL) {
    lv_color_format_t cf = lv_display_get_color_format(disp);
    lv_display_rotate_area(disp, &rotated_area);

    uint32_t src_stride =
        lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);
    uint32_t dest_stride =
        lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);

    int32_t src_w = lv_area_get_width(area);
    int32_t src_h = lv_area_get_height(area);

    lv_draw_sw_rotate(px_map, g_rotated_buf, src_w, src_h, src_stride,
                      dest_stride, rotation, cf);
    px_map = g_rotated_buf;
  }

  int x1 = rotated_area.x1;
  int x2 = rotated_area.x2;
  int y1 = rotated_area.y1;
  int y2 = rotated_area.y2;

  size_t len = (x2 - x1 + 1) * (y2 - y1 + 1) * 2;
  // Align pointer and length to 64 bytes for ESP32-P4 cache line size
  uint32_t start_addr = (uint32_t)px_map;
  uint32_t aligned_start = start_addr & ~63;
  uint32_t offset = start_addr - aligned_start;
  size_t aligned_len = (len + offset + 63) & ~63;

  // Sync the PSRAM/CPU cache before handing it over to the hardware DMA
  esp_cache_msync((void *)aligned_start, aligned_len,
                  ESP_CACHE_MSYNC_FLAG_DIR_C2M);

  esp_lcd_panel_draw_bitmap(panel, x1, y1, x2 + 1, y2 + 1, px_map);
  // lv_display_flush_ready is invoked by example_notify_lvgl_flush_ready
}

static bool
example_notify_lvgl_flush_ready(esp_lcd_panel_handle_t panel,
                                esp_lcd_dpi_panel_event_data_t *edata,
                                void *user_ctx) {
  lv_display_t *disp = (lv_display_t *)user_ctx;
  lv_display_flush_ready(disp);
  return false;
}

void display_init(void) {
  ESP_LOGI(TAG, "Initializing Hardware (Manual LVGL Port)...");

  // 1. Power & backlight (LDO)
  esp_ldo_channel_config_t ldo_cfg = {
      .chan_id = MIPI_DSI_PHY_PWR_LDO_CHAN,
      .voltage_mv = MIPI_DSI_PHY_PWR_LDO_VOLT,
  };
  esp_ldo_channel_handle_t ldo_handle;
  ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_cfg, &ldo_handle));

  // 1b. Backlight GPIO (Redundant 23 & 26 for different board revs)
  gpio_config_t bk_conf = {.pin_bit_mask = (1ULL << 23) | (1ULL << 26),
                           .mode = GPIO_MODE_OUTPUT};
  gpio_config(&bk_conf);
  // Force Backlight ON IMMEDIATELY
  gpio_set_level(23, 1);
  gpio_set_level(26, 1);
  ESP_LOGI(TAG, "Backlight Pins (23, 26) Enabled EARLY.");

  // 1c. GSL-Aware Hardware Reset Sequence (Shared GPIO 27 and INT GPIO 3)
  // GSL3680 needs INT=0 during RESET transition to select I2C address 0x40.
  ESP_LOGI(TAG, "Executing GSL-Aware Reset Sequence (RST:27, INT:3)...");
  gpio_config_t rst_pins_conf = {
      .pin_bit_mask = (1ULL << LCD_PIN_RST) | (1ULL << 3), // 3 is TOUCH_PIN_INT
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
  };
  gpio_config(&rst_pins_conf);

  gpio_set_level(LCD_PIN_RST, 0);
  gpio_set_level(3, 0); // Pull INT low for GSL3680 mode selection
  vTaskDelay(pdMS_TO_TICKS(50));
  gpio_set_level(LCD_PIN_RST, 1);
  vTaskDelay(pdMS_TO_TICKS(120)); // Stabilization

  // 2. DSI Bus
  esp_lcd_dsi_bus_handle_t dsi_bus;
  esp_lcd_dsi_bus_config_t bus_config = {
      .bus_id = 0,
      .num_data_lanes = LCD_MIPI_DSI_LANE_NUM,
      .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
      .lane_bit_rate_mbps = LCD_MIPI_DSI_LANE_BITRATE,
  };
  ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, &dsi_bus));

  // 3. DBI IO
  esp_lcd_panel_io_handle_t io_handle;
  esp_lcd_dbi_io_config_t dbi_config = {
      .virtual_channel = 0, .lcd_cmd_bits = 8, .lcd_param_bits = 8};
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_config, &io_handle));

  // 4. Panel JD9365
  esp_lcd_dpi_panel_config_t dpi_config = {
      .virtual_channel = 0,
      .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
      .dpi_clock_freq_mhz = 60, // Manufacturer 60MHz
      .pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565,
      .num_fbs = 1,
      .video_timing =
          {
              .h_size = 800,
              .v_size = 1280,
              .hsync_pulse_width = 20,
              .hsync_back_porch = 20,
              .hsync_front_porch = 40,
              .vsync_pulse_width = 4,
              .vsync_back_porch = 8,
              .vsync_front_porch = 20,
          },
      .flags.use_dma2d = true,
  };
  jd9365_vendor_config_t vendor_config = {
      .mipi_config = {.dsi_bus = dsi_bus,
                      .dpi_config = &dpi_config,
                      .lane_num = LCD_MIPI_DSI_LANE_NUM},
  };
  esp_lcd_panel_dev_config_t lcd_dev_config = {
      .reset_gpio_num = -1, // RESET IS HANDLED MANUALLY ABOVE
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = 16,
      .vendor_config = &vendor_config,
  };
  ESP_ERROR_CHECK(
      esp_lcd_new_panel_jd9365(io_handle, &lcd_dev_config, &panel_handle));
  // Skip esp_lcd_panel_reset as we did it manually to satisfy both drivers
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  // Ensure Backlight is explicitly ON after init
  gpio_set_level(23, 1);
  gpio_set_level(26, 1);

  // 5. LVGL Display Creation
  // Allocate buffer in SPIRAM (1/5 of screen)
  size_t buffer_size_pixels = LCD_H_RES * LCD_V_RES / 5;
  size_t buffer_size_bytes = buffer_size_pixels * sizeof(uint16_t);

  // Align the buffer size to cache line size (64 bytes on ESP32-P4)
  buffer_size_bytes = (buffer_size_bytes + 63) & ~63;

  void *buf1 = heap_caps_aligned_alloc(64, buffer_size_bytes,
                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
  g_rotated_buf = heap_caps_aligned_alloc(64, buffer_size_bytes,
                                          MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
  // Allocate LVGL buffer
  if (!buf1 || !g_rotated_buf) {
    ESP_LOGE(TAG, "Alloc for LVGL (or rotated) buffer failed!");
    abort();
  }

  // Create LVGL display with PHYSICAL resolution (800x1280)
  lv_display_t *disp =
      lv_display_create(LCD_PHYSICAL_H_RES, LCD_PHYSICAL_V_RES);
  g_lv_display = disp; // store globally for other modules

  // Software rotation to landscape (1280x800 logical)
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

  esp_lcd_dpi_panel_event_callbacks_t cbs = {
      .on_color_trans_done = example_notify_lvgl_flush_ready,
  };
  esp_lcd_dpi_panel_register_event_callbacks(panel_handle, &cbs, disp);

  lv_display_set_user_data(disp, panel_handle);
  lv_display_set_flush_cb(disp, lvgl_dsi_flush);
  lv_display_set_buffers(disp, buf1, NULL, buffer_size_bytes,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  ESP_LOGI(TAG, "Display Initialized Success.");
}

void display_show_energy_meter(void) {
  if (!g_lv_display) {
    ESP_LOGE(TAG, "Cannot show image: Display not initialized");
    return;
  }

  // Lock LVGL if needed (though usually in init we are single threaded or
  // before scheduler) Assuming simple case for now

  lv_obj_t *scr = lv_display_get_screen_active(g_lv_display);

  // Clean screen
  lv_obj_clean(scr);

  lv_obj_t *img = lv_image_create(scr);
  lv_image_set_src(img, &energy_meter);
  lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

  // Force a task handler call or let the loop handle it
}

esp_lcd_panel_handle_t display_get_handle(void) { return panel_handle; }

lv_display_t *display_get_lv_display(void) { return g_lv_display; }

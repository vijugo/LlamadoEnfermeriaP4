#include "touch.h"
#include "display.h"
#include "driver/i2c_master.h"
// Use LOCAL driver header instead of managed component
#include "esp_lcd_touch_gsl3680.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

static const char *TAG = "touch";
static esp_lcd_touch_handle_t touch_handle = NULL;
static i2c_master_bus_handle_t i2c_bus_handle = NULL;

i2c_master_bus_handle_t touch_get_i2c_bus(void) {
  if (i2c_bus_handle == NULL) {
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = TOUCH_I2C_PORT,
        .scl_io_num = TOUCH_PIN_SCL,
        .sda_io_num = TOUCH_PIN_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));
  }
  return i2c_bus_handle;
}

void touch_init(void) {
  ESP_LOGI(TAG, "Initializing Touch (GT911 - Local Driver)...");
  i2c_master_bus_handle_t bus = touch_get_i2c_bus();

  // Configure INT as Input for driver use.
  gpio_config_t touch_int_conf = {
      .pin_bit_mask = (1ULL << TOUCH_PIN_INT),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
  };
  gpio_config(&touch_int_conf);

  esp_lcd_panel_io_handle_t tp_io = NULL;

  // Use GT911 Config from local header
  // Note: GT911 usually needs 0x5D or 0x14. Local header defines macros.
  // Scan for I2C device
  uint8_t touch_addr = 0;
  esp_err_t err;

  // Addresses to probe: 0x5D (GT911 default), 0x14 (GT911 alternate), 0x40
  // (GSL3680)
  uint8_t addresses[] = {0x5D, 0x14, 0x40};

  ESP_LOGI(TAG, "Scanning I2C bus for Touch Controller...");
  for (int i = 0; i < sizeof(addresses) / sizeof(addresses[0]); i++) {
    err = i2c_master_probe(bus, addresses[i], 100);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Found Touch Controller at 0x%02X", addresses[i]);
      touch_addr = addresses[i];
      break;
    }
  }

  if (touch_addr == 0) {
    ESP_LOGE(TAG, "Touch Controller NOT FOUND on I2C bus!");
    // Don't crash, just return to allow system to run without touch
    return;
  }

  if (touch_addr == 0x40) {
    ESP_LOGI(TAG,
             "GSL1680/GSL3680 Controller detected at 0x40! Initializing...");

    esp_lcd_panel_io_i2c_config_t tp_io_cfg =
        ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();
    tp_io_cfg.scl_speed_hz = 100000;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus, &tp_io_cfg, &tp_io));

    esp_lcd_touch_io_gsl3680_config_t gsl_io_cfg = {
        .dev_addr = touch_addr,
    };

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = 1500,
        .y_max = 1500,
        .rst_gpio_num = TOUCH_PIN_RST,
        .int_gpio_num = TOUCH_PIN_INT,
        .flags =
            {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = 0,
            },
        .driver_data = &gsl_io_cfg, // Pass extended config for Pin-Init
    };

    ESP_ERROR_CHECK(
        esp_lcd_touch_new_i2c_gsl3680(tp_io, &tp_cfg, &touch_handle));
  } else {
    // Default to GT911 for 0x5D or 0x14
    esp_lcd_panel_io_i2c_config_t tp_io_cfg =
        ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_cfg.dev_addr = touch_addr;
    tp_io_cfg.scl_speed_hz = 100000;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus, &tp_io_cfg, &tp_io));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_PHYSICAL_H_RES,
        .y_max = LCD_PHYSICAL_V_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = TOUCH_PIN_INT,
        .flags =
            {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = 1,
            },
    };

    ESP_LOGI(TAG, "Creating GT911 instance...");
    esp_err_t gt911_err =
        esp_lcd_touch_new_i2c_gt911(tp_io, &tp_cfg, &touch_handle);
    if (gt911_err != ESP_OK) {
      ESP_LOGE(TAG,
               "Failed to initialize GT911 (err: %s). Operating without touch.",
               esp_err_to_name(gt911_err));
    }
  }

  // Initialize Input Device for LVGL

  const lvgl_port_touch_cfg_t input_cfg = {
      .disp = display_get_lv_display(),
      .handle = touch_handle,
  };
  ESP_LOGI(TAG, "Registering LVGL Input Device...");

  if (touch_handle != NULL) {
    if (lvgl_port_add_touch(&input_cfg) == NULL) {
      ESP_LOGE(TAG, "Failed to register touch input device!");
    }
  } else {
    ESP_LOGW(TAG, "Touch handle is NULL, skipping LVGL touch registration.");
  }
}

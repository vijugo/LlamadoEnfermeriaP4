#include "rtc.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "touch.h"
#include <string.h>

static const char *TAG = "rtc";
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t rtc_dev_handle = NULL;

// RX8025T Register Map
// Note: Some RX8025 datasheets use D3..D0 for address.
// Standard I2C access uses direct addresses.
#define RX8025_REG_SEC 0x00
#define RX8025_REG_MIN 0x01
#define RX8025_REG_HOUR 0x02
#define RX8025_REG_WEEK 0x03
#define RX8025_REG_DAY 0x04
#define RX8025_REG_MONTH 0x05
#define RX8025_REG_YEAR 0x06
#define RX8025_REG_CTRL1 0x0E
#define RX8025_REG_CTRL2 0x0F

static uint8_t bcd2bin(uint8_t val) { return (val & 0x0F) + (val >> 4) * 10; }
static uint8_t bin2bcd(uint8_t val) { return ((val / 10) << 4) + (val % 10); }

esp_err_t rtc_init(void) {
  // Use the bus already initialized by the touch driver
  i2c_bus_handle = touch_get_i2c_bus();

  if (rtc_dev_handle == NULL) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = RTC_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &rtc_dev_handle));
  }

  ESP_LOGI(TAG, "RTC Initialized (RX8025T) on Shared I2C Bus");

  // Check communication
  uint8_t ctrl;
  uint8_t reg =
      (RX8025_REG_CTRL1 << 4); // Some RX8025 need address in high nibble
  // Let's try direct first, then shifted if it fails.
  esp_err_t ret =
      i2c_master_transmit_receive(rtc_dev_handle, &reg, 1, &ctrl, 1, -1);

  if (ret != ESP_OK) {
    // Try with shifted address if direct fails
    reg = (RX8025_REG_CTRL1 << 4);
    ret = i2c_master_transmit_receive(rtc_dev_handle, &reg, 1, &ctrl, 1, -1);
  }

  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "RTC Communication OK. Control Register: 0x%02X", ctrl);
  } else {
    ESP_LOGE(TAG, "Failed to communicate with RTC: %s", esp_err_to_name(ret));
  }

  return ret;
}

esp_err_t rtc_get_time(struct tm *tm) {
  if (rtc_dev_handle == NULL)
    return ESP_ERR_INVALID_STATE;

  uint8_t data[7];
  uint8_t reg = (RX8025_REG_SEC << 4);

  esp_err_t ret =
      i2c_master_transmit_receive(rtc_dev_handle, &reg, 1, data, 7, -1);
  if (ret != ESP_OK)
    return ret;

  tm->tm_sec = bcd2bin(data[0] & 0x7F);
  tm->tm_min = bcd2bin(data[1] & 0x7F);
  tm->tm_hour = bcd2bin(data[2] & 0x3F);
  tm->tm_wday = data[3] & 0x07;
  tm->tm_mday = bcd2bin(data[4] & 0x3F);
  tm->tm_mon = bcd2bin(data[5] & 0x1F) - 1;
  tm->tm_year = bcd2bin(data[6]) + 100;

  return ESP_OK;
}

esp_err_t rtc_set_time(const struct tm *tm) {
  if (rtc_dev_handle == NULL)
    return ESP_ERR_INVALID_STATE;

  uint8_t data[8];
  data[0] = (RX8025_REG_SEC << 4);
  data[1] = bin2bcd(tm->tm_sec);
  data[2] = bin2bcd(tm->tm_min);
  data[3] = bin2bcd(tm->tm_hour);
  data[4] = tm->tm_wday;
  data[5] = bin2bcd(tm->tm_mday);
  data[6] = bin2bcd(tm->tm_mon + 1);
  data[7] = bin2bcd(tm->tm_year % 100);

  return i2c_master_transmit(rtc_dev_handle, data, 8, -1);
}

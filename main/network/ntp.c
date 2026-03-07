#include "ntp.h"
#include "drivers/rtc.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include <sys/time.h>
#include <time.h>

static const char *TAG = "ntp";

void time_sync_notification_cb(struct timeval *tv) {
  ESP_LOGI(TAG, "Notification of a time synchronization event");

  time_t now;
  struct tm timeinfo;
  time(&now);

  // Set timezone to Colombia (GMT-5)
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();

  localtime_r(&now, &timeinfo);

  ESP_LOGI(TAG, "Time synchronized: %04d-%02d-%02d %02d:%02d:%02d",
           timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // Update Hardware RTC
  if (rtc_set_time(&timeinfo) == ESP_OK) {
    ESP_LOGI(TAG, "Hardware RTC Updated with NTP time.");
  } else {
    ESP_LOGE(TAG, "Failed to update Hardware RTC.");
  }
}

void ntp_init(void) {
  ESP_LOGI(TAG, "Initializing SNTP...");
  esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_setservername(1, "time.google.com");
  esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
  esp_sntp_init();
}

void ntp_sync_time(void) {
  // Just a log, since sntp_init already starts the process
  ESP_LOGI(TAG, "NTP Sync requested (WiFi is UP)");
}

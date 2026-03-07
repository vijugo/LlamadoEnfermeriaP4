#include "fast_task.h"
#include "driver/gptimer.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "fast_task";
static TaskHandle_t fast_task_handle = NULL;
static uint32_t execution_count = 0;

// Timer callback executed in ISR context
static bool IRAM_ATTR timer_callback(gptimer_handle_t timer,
                                     const gptimer_alarm_event_data_t *edata,
                                     void *user_data) {
  BaseType_t high_task_awoken = pdFALSE;
  // Notify the fast task to run
  vTaskNotifyGiveFromISR(fast_task_handle, &high_task_awoken);
  return high_task_awoken == pdTRUE;
}

// Low-level task running at 10kHz (every 100us)
static void fast_task_logic(void *pvParameters) {
  ESP_LOGI(TAG, "Fast task started on Core 1");

  while (1) {
    // Wait for notification from ISR
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // --- PERFORMANCE MONITORING ---
    execution_count++;
    if (execution_count % 10000 == 0) { // Every 1 second (10,000 * 100us)
      ESP_LOGD(TAG, "Fast task heartbeat (10k executions)");
    }

    // --- ADD YOUR 100us LOGIC HERE ---
    // CRITICAL: Keep this logic extremely short (< 20us) to avoid starvation
  }
}

void fast_task_init(void) {
  ESP_LOGI(TAG, "Configuring Hardware Timer for 100us intervals...");

  // 1. Create the task with very high priority (pinned to Core 1 to avoid
  // interference with WiFi/UI on Core 0)
  xTaskCreatePinnedToCore(fast_task_logic, "fast_logic", 4096, NULL,
                          configMAX_PRIORITIES - 1, &fast_task_handle, 1);

  // 2. Configure GPTimer
  gptimer_handle_t timer = NULL;
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = 1000000, // 1us per tick
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer));

  // 3. Set Alarm for 100us
  gptimer_alarm_config_t alarm_config = {
      .reload_count = 0,
      .alarm_count = 100, // 100us
      .flags.auto_reload_on_alarm = true,
  };

  gptimer_event_callbacks_t cbs = {
      .on_alarm = timer_callback,
  };
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, NULL));
  ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));

  // 4. Enable and Start
  ESP_ERROR_CHECK(gptimer_enable(timer));
  ESP_ERROR_CHECK(gptimer_start(timer));

  ESP_LOGI(TAG, "Hardware Timer started successfully (100us)");
}

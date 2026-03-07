#include "Aplicacion.h"
#include "drivers/sd_card.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "network/wifi.h"
#include "ui/ui.h"
#include "ui/ui_events.h"
#include "ui/ui_manual_helpers.h"

static const char *TAG = "aplicacion";
static aplicacion_estado_t estado_actual = ST_APLICACION_ARRANQUE;
static uint32_t ticks_en_estado = 0;

// Versión del Sistema
const char *VERSION_APP = "1.0.0";
const char *FECHA_APP = "Mar 06 2026";

void aplicacion_loop_task(void *pvParameters) {
  ESP_LOGI(TAG, "Tarea de Aplicación Iniciada.");

  while (1) {
    ticks_en_estado++;

    switch (estado_actual) {
    case ST_APLICACION_ARRANQUE:
      if (ticks_en_estado == 1) {
        ESP_LOGI(TAG, "Estado: ARRANQUE. Mostrando SquareLine Screen1");
        ui_init();
      }

      // Espera 5 segundos (5000ms / 100ms = 50 ticks)
      if (ticks_en_estado >= 50) {
        estado_actual = ST_APLICACION_ESPERA;
        ticks_en_estado = 0;
      }
      break;

    case ST_APLICACION_ESPERA:
      if (ticks_en_estado == 1) {
        ESP_LOGI(TAG, "Estado: ESPERA. Mostrando EsperaLlamado.png");
        ui_show_screen_espera();
      }

      // Actualizar info de red y SD cada segundo (10 ticks)
      if (ticks_en_estado % 10 == 0) {
        ui_update_espera_data(wifi_is_connected(), sd_card_is_mounted());
      }

      // Manejar eventos de UI desde SquareLine
      if (F_Configurar) {
        ESP_LOGI(TAG, "Evento: F_Configurar detectado.");
        estado_actual = ST_APLICACION_PARA;
        ticks_en_estado = 0;
        F_Configurar = false; // Reset flag
      }
      break;

    case ST_APLICACION_PARA:
      if (ticks_en_estado == 1) {
        ESP_LOGI(TAG, "Estado: PARA (Configuración). Mostrando Screen3");
        ui_show_screen_para();
      }

      if (F_Retornar_Llamado) {
        ESP_LOGI(TAG,
                 "Evento: F_Retornar_Llamado detectado. Volviendo a ESPERA.");
        estado_actual = ST_APLICACION_ESPERA;
        ticks_en_estado = 0;
        F_Retornar_Llamado = false;
      }
      break;

    case ST_APLICACION_RETARDO:
    case ST_APLICACION_TEXTOSPEECH:
    case ST_APLICACION_PRUEBA:
    default:
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Tick de 100ms
  }
}

void aplicacion_init(void) {
  ESP_LOGI(TAG, "Iniciando Máquina de Estados...");
  xTaskCreate(aplicacion_loop_task, "aplicacion_task", 16384, NULL, 5, NULL);
}

void aplicacion_notificar_evento_ui(int evento) {
  // Obsoleto, usamos flags de ui_events.h
}

aplicacion_estado_t aplicacion_get_estado(void) { return estado_actual; }

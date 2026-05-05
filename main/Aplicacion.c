#include "Aplicacion.h"
#include "drivers/codec.h"
#include "drivers/rtc.h"
#include "drivers/sd_card.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "network/wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "ui/ui.h"
#include "ui/ui_events.h"
#include "ui/ui_manual_helpers.h"
#include <stdio.h>

static const char *TAG = "aplicacion";
static aplicacion_estado_t estado_actual = ST_APLICACION_ARRANQUE;
static uint32_t ticks_en_estado = 0;

// Versión del Sistema
const char *VERSION_APP = "1.0.0";
const char *FECHA_APP = "Mar 06 2026";

static int volumen_actual = 50;

void aplicacion_loop_task(void *pvParameters) {
  ESP_LOGI(TAG, "Tarea de Aplicación Iniciada.");

  while (1) {
    ticks_en_estado++;

    switch (estado_actual) {
    case ST_APLICACION_ARRANQUE:
      if (ticks_en_estado == 1) {
        ESP_LOGI(TAG, "Estado: ARRANQUE. Esperando...");
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

      if (F_Configura_Parmetros) {
        ESP_LOGI(TAG, "Evento: F_Configura_Parmetros detectado.");
        estado_actual = ST_APLICACION_CONFIGURAR_PARAMETROS;
        ticks_en_estado = 0;
        F_Configura_Parmetros = false;
      }

      if (F_Historial) {
        ESP_LOGI(TAG, "Evento: F_Historial detectado.");
        estado_actual = ST_APLICACION_HISTORIAL;
        ticks_en_estado = 0;
        F_Historial = false;
      }
      break;

    case ST_APLICACION_CONFIGURAR_PARAMETROS:
      if (ticks_en_estado == 1) {
        ESP_LOGI(TAG, "Estado: CONFIGURAR_PARAMETROS. Cargando valores RTC.");
        F_Salir_Parametros = false;
        F_Grabar_Parametros = false;
        ui_show_screen_config_parametros();

        struct tm now;
        if (rtc_get_time(&now) == ESP_OK) {
          ui_set_config_values(now.tm_mday, now.tm_mon + 1, now.tm_year + 1900,
                               now.tm_hour, now.tm_min, volumen_actual);
        } else {
          ui_set_config_values(1, 1, 2026, 12, 0, volumen_actual);
        }
      }

      if (F_Salir_Parametros) {
        ESP_LOGI(TAG, "Evento: F_Salir_Parametros detectado. Cancelando...");
        estado_actual = ST_APLICACION_PARA;
        ticks_en_estado = 0;
        F_Salir_Parametros = false;
        ui_show_screen_para();
      }

      if (F_Grabar_Parametros) {
        ESP_LOGI(TAG, "Evento: F_Grabar_Parametros detectado. Guardando...");
        int d, m, y, h, min, v;
        ui_get_config_values(&d, &m, &y, &h, &min, &v);

        struct tm new_time;
        memset(&new_time, 0, sizeof(struct tm));
        new_time.tm_mday = d;
        new_time.tm_mon = m - 1;
        new_time.tm_year = y - 1900;
        new_time.tm_hour = h;
        new_time.tm_min = min;
        new_time.tm_sec = 0;

        rtc_set_time(&new_time);
        volumen_actual = v;
        codec_set_volume(volumen_actual);

        nvs_handle_t my_handle;
        if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
          nvs_set_i32(my_handle, "volume", volumen_actual);
          nvs_commit(my_handle);
          nvs_close(my_handle);
        }

        ESP_LOGI(TAG, "Parámetros grabados: %02d/%02d/%04d %02d:%02d, Vol: %d",
                 d, m, y, h, min, v);

        estado_actual = ST_APLICACION_PARA;
        ticks_en_estado = 0;
        F_Grabar_Parametros = false;
        ui_show_screen_para();
      }
      break;

    case ST_APLICACION_HISTORIAL:
      if (ticks_en_estado == 1) {
        ESP_LOGI(TAG, "Estado: HISTORIAL. (Pantalla en desarrollo)");
        // Por ahora volvemos a PARA después de 3 segundos
      }
      if (ticks_en_estado >= 30) {
        ESP_LOGI(TAG, "Volviendo a PARA desde HISTORIAL.");
        estado_actual = ST_APLICACION_PARA;
        ticks_en_estado = 0;
        ui_show_screen_para();
      }
      break;

    case ST_APLICACION_RETARDO:
      break;

    case ST_APLICACION_TEXTOSPEECH:
      if (ticks_en_estado == 1) {
        ESP_LOGI(TAG, "Estado: TEXTOSPEECH. Iniciando motor de voz...");
      }
      // Placeholder for TTS logic
      if (ticks_en_estado >= 20) {
        estado_actual = ST_APLICACION_ESPERA;
        ticks_en_estado = 0;
      }
      break;

    case ST_APLICACION_PRUEBA:
    default:
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Tick de 100ms
  }
}

void aplicacion_init(void) {
  ESP_LOGI(TAG, "SPIFFS ya inicializado desde main.c");

  // Cargar volumen de NVS
  nvs_handle_t lv_my_handle;
  if (nvs_open("storage", NVS_READONLY, &lv_my_handle) == ESP_OK) {
    int32_t val;
    if (nvs_get_i32(lv_my_handle, "volume", &val) == ESP_OK) {
      volumen_actual = (int)val;
      ESP_LOGI(TAG, "Volumen recuperado de NVS: %d", volumen_actual);
    }
    nvs_close(lv_my_handle);
  }
  codec_set_volume(volumen_actual);

  ESP_LOGI(TAG, "Iniciando Máquina de Estados...");
  xTaskCreate(aplicacion_loop_task, "aplicacion_task", 16384, NULL, 5, NULL);
}

void aplicacion_notificar_evento_ui(int evento) {
  // Obsoleto, usamos flags de ui_events.h
}

aplicacion_estado_t aplicacion_get_estado(void) { return estado_actual; }

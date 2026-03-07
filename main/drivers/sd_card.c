#include "sd_card.h"
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "esp_ldo_regulator.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdmmc_cmd.h"
#include <dirent.h>
#include <sys/stat.h>

static const char *TAG = "sd_card";
static bool mounted = false;

#define MOUNT_POINT "/sd"

esp_err_t sd_card_init(void) {
  esp_err_t ret;

  ESP_LOGI(TAG, "Activando alimentación para MicroSD (VDDPST_5 / LDO 4)...");

  // Activar el LDO Channel 4 (correspondiente a VDDPST_5 según diseño P4)
  esp_ldo_channel_config_t ldo_cfg = {
      .chan_id = 4,
      .voltage_mv = 3300, // 3.3V para la tarjeta SD
  };
  esp_ldo_channel_handle_t ldo_handle;
  ret = esp_ldo_acquire_channel(&ldo_cfg, &ldo_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error al activar LDO 4 (Power Domain VDDPST_5): %s",
             esp_err_to_name(ret));
    return ret;
  }
  ESP_LOGI(TAG, "Alimentación (3.3V) activada en LDO 4.");

  // Longer wait for voltage stabilization
  vTaskDelay(pdMS_TO_TICKS(500));

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 64 * 1024};

  sdmmc_card_t *card;
  const char mount_point[] = MOUNT_POINT;

  ESP_LOGI(
      TAG,
      "Inicializando MicroSD (Slot 0) con pines: CLK:43, CMD:44, D0-D3:39-42");

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.slot = SDMMC_HOST_SLOT_0;
  host.max_freq_khz = SDMMC_FREQ_DEFAULT; // 20MHz or 40MHz

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.clk = GPIO_NUM_43;
  slot_config.cmd = GPIO_NUM_44;
  slot_config.d0 = GPIO_NUM_39;
  slot_config.d1 = GPIO_NUM_40;
  slot_config.d2 = GPIO_NUM_41;
  slot_config.d3 = GPIO_NUM_42;
  slot_config.width = 4;
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  ESP_LOGI(TAG, "Montando sistema de archivos FAT en %s...", mount_point);

  // Try mounting with retries
  int retry_count = 3;
  while (retry_count > 0) {
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config,
                                  &mount_config, &card);
    if (ret == ESP_OK)
      break;

    ESP_LOGW(TAG,
             "Intento fallido (%s). Reintentando en 500ms... (%d restantes)",
             esp_err_to_name(ret), retry_count - 1);
    vTaskDelay(pdMS_TO_TICKS(500));

    // If second attempt fails, try 1-bit mode or lower frequency
    if (retry_count == 2) {
      ESP_LOGI(TAG, "Intentando con frecuencia reducida (12MHz)...");
      host.max_freq_khz = 12000;
    }
    retry_count--;
  }

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "No se pudo montar la tarjeta SD tras varios intentos (%s).",
             esp_err_to_name(ret));
    mounted = false;
    return ret;
  }

  mounted = true;
  ESP_LOGI(TAG, "¡Tarjeta SD montada exitosamente!");
  sdmmc_card_print_info(stdout, card);

  // Crear/Actualizar archivo "SD"
  FILE *f = fopen("/sd/SD", "w");
  if (f != NULL) {
    fprintf(f, "MicroSD Inicializada y Alimentada correctamente.\n");
    fclose(f);
    ESP_LOGI(TAG, "Archivo /sd/SD creado.");
  }

  // --- ESCANEO DE LA RAÍZ (Tarea solicitada) ---
  ESP_LOGI(TAG, "Estructura de la carpeta raíz:");
  DIR *dir = opendir(mount_point);
  if (dir) {
    struct dirent *ent;
    char full_path[512];
    while ((ent = readdir(dir)) != NULL) {
      struct stat st;
      snprintf(full_path, sizeof(full_path), "%s/%s", mount_point, ent->d_name);
      if (stat(full_path, &st) == 0) {
        const char *type = S_ISDIR(st.st_mode) ? "DIR " : "FILE";
        ESP_LOGI(TAG, "  [%s] %-20s  %-10ld bytes", type, ent->d_name,
                 (long)st.st_size);
      }
    }
    closedir(dir);
  }

  // --- ESCANEO DE LA CARPETA '01' ---
  char folder_01[32];
  snprintf(folder_01, sizeof(folder_01), "%s/01", mount_point);
  ESP_LOGI(TAG, "Explorando carpeta: %s", folder_01);
  dir = opendir(folder_01);
  if (dir) {
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
      ESP_LOGI(TAG, "  -> %s", ent->d_name);
    }
    closedir(dir);
  }

  return ESP_OK;
}

bool sd_card_is_mounted(void) { return mounted; }

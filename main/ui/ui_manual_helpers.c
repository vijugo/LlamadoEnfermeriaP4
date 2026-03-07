#include "drivers/rtc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_vfs_fat.h"
#include "ff.h"
#include "network/wifi.h"
#include "ui.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

// Helper functions that existed in the previous manual ui.c
// but are missing in the SquareLine-generated one.
// We provide these to keep other modules (like wifi.c) compiling.

void ui_set_wifi_status(bool connected, const char *ssid) {
  // SquareLine Screen1 has 'ui_Label1'. We can maybe update it.
  if (ui_Label1) {
    // Just for example, we don't know the layout yet.
  }
}

void ui_update_espera_data(bool wifi, bool sd) {
  // Variables estáticas para guardar la info y no calcular cada segundo cosas
  // lentas
  static uint32_t last_sd_check = 0;
  static char cached_sd_info[32] = "-- / --";
  uint32_t now_ticks = xTaskGetTickCount();

  // 1. Cálculos LENTOS (FUERA del bloqueo de pantalla)

  // Hora y Fecha
  struct tm now_tm;
  char time_str[10] = "";
  char date_str[64] = "";
  bool rtc_ok = (rtc_get_time(&now_tm) == ESP_OK);
  if (rtc_ok) {
    snprintf(time_str, sizeof(time_str), "%02d:%02d", now_tm.tm_hour,
             now_tm.tm_min);
    const char *days[] = {"Domingo", "Lunes",   "Martes", "Miercoles",
                          "Jueves",  "Viernes", "Sabado"};
    const char *months[] = {"Enero",      "Febrero", "Marzo",     "Abril",
                            "Mayo",       "Junio",   "Julio",     "Agosto",
                            "Septiembre", "Octubre", "Noviembre", "Diciembre"};
    snprintf(date_str, sizeof(date_str), "%s, %d de %s de %d",
             days[now_tm.tm_wday], now_tm.tm_mday, months[now_tm.tm_mon],
             now_tm.tm_year + 1900);
  }

  // SD Espacio (Solo cada 30 segundos porque f_getfree es pesadísimo)
  if (sd && (now_ticks - last_sd_check > pdMS_TO_TICKS(30000) ||
             last_sd_check == 0)) {
    FATFS *fs;
    DWORD fre_clust, fre_sect, tot_sect;
    // Esto puede tardar segunos, por eso se hace FUERA del lock
    if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
      tot_sect = (fs->n_fatent - 2) * fs->csize;
      fre_sect = fre_clust * fs->csize;
      double tot_gb = (double)tot_sect * 512 / (1024.0 * 1024.0 * 1024.0);
      double fre_gb = (double)fre_sect * 512 / (1024.0 * 1024.0 * 1024.0);
      snprintf(cached_sd_info, sizeof(cached_sd_info), "%.1fG / %.1fG",
               (tot_gb - fre_gb), tot_gb);
    } else {
      snprintf(cached_sd_info, sizeof(cached_sd_info), "Error FAT");
    }
    last_sd_check = now_ticks;
  } else if (!sd) {
    snprintf(cached_sd_info, sizeof(cached_sd_info), "-- / --");
    last_sd_check = 0;
  }

  // 2. ACTUALIZACIÓN RÁPIDA (DENTRO del bloqueo de pantalla)
  if (lvgl_port_lock(0)) {
    if (ui_Screen2 == NULL) {
      lvgl_port_unlock();
      return;
    }

    if (rtc_ok) {
      if (ui_Label2)
        lv_label_set_text(ui_Label2, time_str);
      if (ui_Label3)
        lv_label_set_text(ui_Label3, date_str);
    }

    if (ui_Label4) {
      lv_label_set_text(ui_Label4, wifi ? WIFI_SSID : "Sin Red");
    }

    if (ui_Label5) {
      lv_label_set_text(ui_Label5, wifi ? "Conectada" : "Desconectada");
      lv_obj_set_style_text_color(ui_Label5,
                                  wifi ? lv_palette_main(LV_PALETTE_GREEN)
                                       : lv_palette_main(LV_PALETTE_RED),
                                  0);
    }

    if (ui_Label6) {
      lv_label_set_text(ui_Label6, sd ? "Insertada" : "No detectada");
      lv_obj_set_style_text_color(ui_Label6,
                                  sd ? lv_palette_main(LV_PALETTE_GREEN)
                                     : lv_palette_main(LV_PALETTE_RED),
                                  0);
    }

    if (ui_Label7) {
      lv_label_set_text(ui_Label7, cached_sd_info);
    }

    lvgl_port_unlock();
  }
}

void ui_show_screen_arranque(const char *version, const char *fecha) {
  // In SquareLine, this is ui_init()
}

static void global_touch_monitor_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    lv_indev_t *indev = lv_indev_get_act();
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    ESP_LOGW("TOUCH_HW", "SCREEN TOUCH at X:%d, Y:%d", (int)p.x, (int)p.y);
  }
}

void ui_show_screen_espera(void) {
  if (lvgl_port_lock(0)) {
    if (ui_Screen2 == NULL) {
      ESP_LOGI("UI_HELPERS", "Initializing Screen2...");
      ui_Screen2_screen_init();
    }
    // Aseguramos que el botón sea clickeable por si SquareLine lo dejó solo
    // como imagen
    if (ui_Button1) {
      lv_obj_add_flag(ui_Button1, LV_OBJ_FLAG_CLICKABLE);
    }

    // Monitor global de toques para ver coordenadas
    lv_obj_remove_event_cb(ui_Screen2, global_touch_monitor_cb);
    lv_obj_add_event_cb(ui_Screen2, global_touch_monitor_cb, LV_EVENT_PRESSED,
                        NULL);

    ESP_LOGI("UI_HELPERS", "Loading Screen2...");
    lv_disp_load_scr(ui_Screen2);
    lvgl_port_unlock();
  }
}

void ui_show_screen_para(void) {
  if (lvgl_port_lock(0)) {
    ESP_LOGW("MEM", "Before Screen3 - Free: %u, Internal Free: %u",
             (unsigned int)esp_get_free_heap_size(),
             (unsigned int)esp_get_free_internal_heap_size());
    if (ui_Screen3 == NULL) {
      ESP_LOGI("UI_HELPERS", "Initializing Screen3...");
      ui_Screen3_screen_init();
    }
    ESP_LOGI("UI_HELPERS", "Loading Screen3...");
    lv_disp_load_scr(ui_Screen3);
    ESP_LOGW("MEM", "After Screen3 - Free: %u, Internal Free: %u",
             (unsigned int)esp_get_free_heap_size(),
             (unsigned int)esp_get_free_internal_heap_size());
    lvgl_port_unlock();
  }
}

#ifndef DISPLAY_H
#define DISPLAY_H

#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

// Hardware Pinout
#define LCD_H_RES 1280
#define LCD_V_RES 800
#define LCD_PHYSICAL_H_RES 800
#define LCD_PHYSICAL_V_RES 1280
#define LCD_MIPI_DSI_LANE_NUM 2
#define LCD_MIPI_DSI_LANE_BITRATE 1500
#define LCD_PIN_RST 27
#define LCD_PIN_BL 23
#define MIPI_DSI_PHY_PWR_LDO_CHAN 3
#define MIPI_DSI_PHY_PWR_LDO_VOLT 2500

// Function Declarations
void display_init(void);
esp_lcd_panel_handle_t display_get_handle(void);
lv_display_t *display_get_lv_display(void);

LV_IMG_DECLARE(energy_meter);
void display_show_energy_meter(void);

#endif // DISPLAY_H

#ifndef TOUCH_H
#define TOUCH_H

#include "driver/i2c_master.h"
#include "esp_lcd_touch.h"
#include "lvgl.h"

// Touch Pins
#define TOUCH_I2C_PORT 1
#define TOUCH_PIN_SDA 7
#define TOUCH_PIN_SCL 8
#define TOUCH_PIN_RST 22
#define TOUCH_PIN_INT 21

void touch_init(void);
esp_lcd_touch_handle_t touch_get_handle(void);
i2c_master_bus_handle_t touch_get_i2c_bus(void);

#endif // TOUCH_H

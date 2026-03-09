idf_component_register(SRCS "main.c" "Aplicacion.c" "ui/ui.c" "ui/ui_helpers.c" "ui/ui_events.c" "ui/ui_Screen1.c" "ui/ui_Screen2.c" "ui/ui_Screen3.c" "ui/ui_Screen4.c" "ui/ui_font_Montserrat160.c" "ui/ui_manual_helpers.c" "ui/psram_images.c"
                             "drivers/display.c" "drivers/touch.c" "drivers/esp_lcd_touch_gt911.c" "drivers/fast_task.c" "drivers/sd_card.c" "drivers/codec.c" "drivers/rtc.c" "network/wifi.c" "network/ntp.c"
                       INCLUDE_DIRS "." "ui" "drivers" "network"
                       REQUIRES esp_lcd driver esp_jpeg esp_lcd_jd9365 lvgl esp_lvgl_port nvs_flash sdmmc fatfs esp_codec_dev esp_lcd_touch_gsl3680 esp_wifi esp_event esp_mm spiffs)

spiffs_create_partition_image(storage ../data)

#ifndef PSRAM_IMAGES_H
#define PSRAM_IMAGES_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_image_dsc_t *img_espera;
extern const lv_image_dsc_t *img_plantilla_01;
extern const lv_image_dsc_t *img_f_configura;
extern const lv_image_dsc_t *img_fondohospital;
extern const lv_image_dsc_t *img_b_configuracion;
extern const lv_image_dsc_t *img_b_historial;
extern const lv_image_dsc_t *img_b_llamado;
extern const lv_image_dsc_t *img_botongrandeerror;
extern const lv_image_dsc_t *img_botongrandeok;
extern const lv_image_dsc_t *img_gear;

void init_psram_images(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

#ifndef APLICACION_H
#define APLICACION_H

#include "esp_err.h"
#include <stdbool.h>

typedef enum {
  ST_APLICACION_ARRANQUE,
  ST_APLICACION_ESPERA,
  ST_APLICACION_RETARDO,
  ST_APLICACION_LEE_ARCHIVO,
  ST_APLICACION_SUENA,
  ST_APLICACION_PARA,
  ST_APLICACION_TEXTOSPEECH,
  ST_APLICACION_CONFIGURAR_PARAMETROS,
  ST_APLICACION_HISTORIAL,
  ST_APLICACION_PRUEBA
} aplicacion_estado_t;

/**
 * @brief Inicializa la lógica principal de la aplicación Llamado de Enfermería.
 */
void aplicacion_init(void);

/**
 * @brief Obtiene el estado actual de la aplicación.
 */
aplicacion_estado_t aplicacion_get_estado(void);

#endif // APLICACION_H

# LlamadoEnfermeria v1.0.0


Este proyecto muestra cómo inicializar una pantalla MIPI-DSI de 10.1" (1280x800) en el ESP32-P4.

## Configuración de Hardware (¡CRÍTICO!)
Antes de compilar, debes verificar y ajustar los pines en `main/main.c`:

- `LCD_PIN_BL`: Pin del Backlight
- `LCD_PIN_RST`: Pin de Reset (si aplica)

También verifica la configuración del panel en `dpi_config`:
- `dpi_clock_freq_mhz`: Frecuencia del reloj de píxeles (usualmente 60-80MHz para 1280x800@60Hz)
- `video_timing`: Porches y anchos de pulso (dependen del datasheet de tu pantalla).

## Configuración del Proyecto (menuconfig)
Es necesario habilitar PSRAM (SPIRAM) ya que el framebuffer de 1280x800 requiere ~2MB.

1. Ejecuta:
   ```bash
   idf.py menuconfig
   ```
2. Ve a `Component config` > `ESP PSRAM`.
3. Habilita `Support for external, SPI-connected RAM`.
4. Elige el modo apropiado (Octal SPI usualmente para P4).

## Compilación y Flasheo

1. Compilar:
   ```bash
   idf.py build
   ```
2. Flashear:
   ```bash
   idf.py -p /dev/cu.usbserial-XXXX flash monitor
   ```

## Mostrar una Imagen Real (JPEG)
El código actual muestra barras de colores de prueba. Para mostrar una imagen:
1. Convierte tu imagen a 1280x800.
2. Usa `esp_jpeg` para decodificarla desde una partición SPIFFS o embebida en el código.

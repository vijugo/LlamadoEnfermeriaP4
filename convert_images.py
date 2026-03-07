from PIL import Image
import sys
import os

def convert_to_rgb565_c(img_path, var_name, out_path):
    img = Image.open(img_path).convert('RGB')
    w, h = img.size
    
    with open(out_path, 'w') as f:
        f.write('#include "lvgl.h"\n\n')
        f.write(f'const uint8_t {var_name}_map[] = {{\n')
        
        pixel_data = list(img.getdata())
        for i, (r, g, b) in enumerate(pixel_data):
            # RGB565 (16 bit) - Standard format for LVGL
            # Low 5 bits b, middle 6 bits g, high 5 bits r.
            # In memory ESP32: Little Endian - byte0=G[2:0]B[4:0], byte1=R[4:0]G[5:3]
            # LVGL expects the raw byte array
            # We'll use RGB565 format (2 bytes per pixel)
            c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)
            b0 = c & 0xFF
            b1 = (c >> 8) & 0xFF
            
            f.write(f"0x{b0:02x}, 0x{b1:02x}, ")
            if (i + 1) % 12 == 0:
                f.write("\n  ")
        
        f.write('\n};\n\n')
        f.write(f'const lv_image_dsc_t {var_name} = {{\n')
        f.write('  .header.cf = LV_COLOR_FORMAT_RGB565,\n')
        f.write('  .header.magic = LV_IMAGE_HEADER_MAGIC,\n')
        f.write(f'  .header.w = {w},\n')
        f.write(f'  .header.h = {h},\n')
        f.write(f'  .data_size = {w * h * 2},\n')
        f.write(f'  .data = {var_name}_map,\n')
        f.write('};\n')

convert_to_rgb565_c("Imagenes/Plantilla.png", "img_plantilla_full", "main/ui/plantilla_full.c")
convert_to_rgb565_c("Imagenes/EsperaLlamado.png", "img_espera_full", "main/ui/espera_full.c")

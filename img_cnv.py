from PIL import Image
import sys
import os

def convert_image(input_path, output_path, name):
    img = Image.open(input_path)
    img = img.resize((400, 640)) # Resize to half screen to save flash space (800x1280 is huge for raw bitmaps)
    # Or keep resolution but use CF_TRUE_COLOR_ALPHA? No, raw RGB565 is best.
    # ESP32-P4 has 16MB flash, 800x1280x2 = 2MB. It fits!
    # Let's keep resolution but convert to RGB565 (16bit)
    
    # Reload original full size
    img = Image.open(input_path) 
    # Force 320x320 for test? No, let's try 400x600 for menu area to be safe on flash size
    img = img.resize((600, 600)) 
    img = img.convert("RGB")
    
    with open(output_path, 'w') as f:
        f.write(f'#include "lvgl.h"\n\n')
        f.write(f'const uint8_t {name}_map[] = {{\n')
        
        pixels = list(img.getdata())
        for r, g, b in pixels:
            # RGB888 to RGB565
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            # Little Endian for ESP32
            low = rgb565 & 0xFF
            high = (rgb565 >> 8) & 0xFF
            f.write(f'0x{low:02x}, 0x{high:02x}, ')
            
        f.write('};\n\n')
        
        f.write(f'const lv_image_dsc_t {name} = {{\n')
        f.write('  .header.cf = LV_COLOR_FORMAT_RGB565,\n')
        f.write(f'  .header.w = {img.width},\n')
        f.write(f'  .header.h = {img.height},\n')
        f.write(f'  .data_size = {len(pixels) * 2},\n')
        f.write(f'  .data = {name}_map,\n')
        f.write('};\n')

if __name__ == "__main__":
    convert_image(sys.argv[1], sys.argv[2], sys.argv[3])

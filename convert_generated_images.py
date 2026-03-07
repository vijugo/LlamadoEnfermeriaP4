from PIL import Image
import sys
import os

def convert_to_rgb565_c(img_path, var_name, out_path):
    # Open and SCALE to the target display resolution 1280x800
    img = Image.open(img_path).convert('RGB')
    img = img.resize((1280, 800), Image.Resampling.LANCZOS)
    w, h = img.size
    
    with open(out_path, 'w') as f:
        f.write('#include "lvgl.h"\n\n')
        f.write(f'const uint8_t {var_name}_map[] = {{\n')
        
        pixel_data = list(img.getdata())
        for i, (r, g, b) in enumerate(pixel_data):
            # RGB565 format
            c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)
            b0 = c & 0xFF
            b1 = (c >> 8) & 0xFF
            
            f.write(f"0x{b0:02x}, 0x{b1:02x}, ")
            # Print newline every 12 pixels (24 bytes) for code readability
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

# Convert and RESIZE generated images
convert_to_rgb565_c("/Users/vijugo/.gemini/antigravity/brain/a3628c68-8cb3-497c-9c6a-372b3ef9378e/hospital_startup_bg_1772818107919.png", "hospital_startup_bg", "main/ui/hospital_startup_bg.c")
convert_to_rgb565_c("/Users/vijugo/.gemini/antigravity/brain/a3628c68-8cb3-497c-9c6a-372b3ef9378e/hospital_idle_dashboard_bg_1772818282789.png", "hospital_idle_dashboard_bg", "main/ui/hospital_idle_dashboard_bg.c")
print("Conversion complete: images resized to 1280x800")

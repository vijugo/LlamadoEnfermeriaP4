import sys
import os
from PIL import Image

def convert_to_c_array(input_path, output_c_path, name, scale=1.0):
    try:
        img = Image.open(input_path)
        img = img.convert('RGB')
        
        # Target resolution (PHYSICAL)
        TARGET_W = int(800 * scale)
        TARGET_H = int(1280 * scale)
        
        # Resize/Crop
        img_ratio = img.width / img.height
        target_ratio = TARGET_W / TARGET_H
        
        if img_ratio > target_ratio:
            new_height = TARGET_H
            new_width = int(new_height * img_ratio)
            img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
            left = (new_width - TARGET_W) // 2
            img = img.crop((left, 0, left + TARGET_W, TARGET_H))
        else:
            new_width = TARGET_W
            new_height = int(new_width / img_ratio)
            img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
            top = (new_height - TARGET_H) // 2
            img = img.crop((0, top, TARGET_W, top + TARGET_H))
            
        print(f"Processed {input_path} to {img.width}x{img.height}")
        
        # RGB565 conversion
        byte_data = bytearray()
        pixels = list(img.getdata())
        for r, g, b in pixels:
            val = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            byte_data.append(val & 0xFF)
            byte_data.append((val >> 8) & 0xFF)
            
        # Write C file
        with open(output_c_path, 'w') as f:
            f.write('#include "lvgl.h"\n\n')
            f.write(f'const uint8_t {name}_map[] = {{\n')
            for i in range(0, len(byte_data), 12):
                chunk = byte_data[i:i+12]
                hex_str = ', '.join([f'0x{b:02x}' for b in chunk])
                f.write(f'  {hex_str},\n')
            f.write('};\n\n')
            f.write(f'const lv_image_dsc_t {name} = {{\n')
            f.write('  .header.cf = LV_COLOR_FORMAT_RGB565,\n')
            f.write('  .header.magic = LV_IMAGE_HEADER_MAGIC,\n')
            f.write(f'  .header.w = {img.width},\n')
            f.write(f'  .header.h = {img.height},\n')
            f.write(f'  .data_size = {len(byte_data)},\n')
            f.write(f'  .data = {name}_map,\n')
            f.write('};\n')
        print(f"Done! Created {output_c_path} ({len(byte_data)} bytes)")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python img_to_c.py <input> <output_c> <variable_name> [scale]")
    else:
        scale = float(sys.argv[4]) if len(sys.argv) > 4 else 1.0
        convert_to_c_array(sys.argv[1], sys.argv[2], sys.argv[3], scale)

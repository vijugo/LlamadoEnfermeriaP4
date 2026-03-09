import os
import re

def convert():
    ui_dir = "main/ui"
    data_dir = "data"
    
    if not os.path.exists(data_dir):
        os.makedirs(data_dir)
        print(f"Directorio creado: {data_dir}")

    # Buscar archivos de imagen .c
    files = [f for f in os.listdir(ui_dir) if f.startswith("ui_img_") and f.endswith(".c")]
    
    for filename in files:
        filepath = os.path.join(ui_dir, filename)
        # Nombre simplificado para el archivo binario
        bin_name = filename.replace("ui_img_", "").replace("_png.c", ".bin").lower()
        bin_filepath = os.path.join(data_dir, bin_name)
        
        print(f"Procesando {filename} -> {bin_name}...")
        
        try:
            with open(filepath, 'r', encoding='latin-1') as f:
                content = f.read()
                
            # Buscar el array de datos
            match = re.search(r'data\[\]\s*=\s*\{(.*?)\};', content, re.DOTALL)
            if match:
                hex_data = match.group(1)
                # Extraer los bytes
                bytes_list = re.findall(r'0[xX][0-9a-fA-F]+', hex_data)
                
                with open(bin_filepath, 'wb') as bin_f:
                    byte_array = bytearray([int(b, 16) for b in bytes_list])
                    bin_f.write(byte_array)
                print(f"  -> OK: {len(byte_array)} bytes")
            else:
                print(f"  !! Error: No se encontró array en {filename}")
        except Exception as e:
            print(f"  !! Error procesando {filename}: {e}")

if __name__ == "__main__":
    convert()

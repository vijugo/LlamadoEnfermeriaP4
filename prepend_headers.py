import os
import struct

MAGIC = 0x19
CF = 0x14 # LV_COLOR_FORMAT_RGB565A8 (3 bytes: 2 color + 1 alpha)

images = {
    "espera.bin": (1280, 800),
    "f_configura.bin": (1280, 796),
    "fondohospital.bin": (1280, 800),
    "plantilla_01.bin": (1280, 800),
    "b_configuracion.bin": (205, 207),
    "b_historial.bin": (208, 207),
    "b_llamado.bin": (205, 208),
    "botongrandeerror.bin": (255, 255),
    "botongrandeok.bin": (255, 255),
    "gear_8248448.bin": (200, 200)
}

def process():
    data_dir = "data"
    for filename, (w, h) in images.items():
        path = os.path.join(data_dir, filename)
        if not os.path.exists(path):
            continue
            
        with open(path, 'rb') as f:
            data = f.read()
        
        # Eliminar cabecera previa si existe (12 bytes)
        if data[0] == MAGIC:
             data = data[12:]
            
        # Stride para RGB565A8 es Ancho * 2 (plano de color)
        stride = w * 2
        
        header = struct.pack('<BBHHHHH', MAGIC, CF, 0, w, h, stride, 0)
        
        with open(path, 'wb') as f:
            f.write(header)
            f.write(data)
        print(f"Format RGB565A8 (0x14) restored for {filename}")

if __name__ == "__main__":
    process()

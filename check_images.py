from PIL import Image
import os
import sys

def check_img(p):
    try:
        with Image.open(p) as img:
            print(f"{p}: {img.format} {img.size} {img.mode}")
    except Exception as e:
        print(f"Error {p}: {e}")

check_img("Imagenes/Plantilla.png")
check_img("Imagenes/EsperaLlamado.png")

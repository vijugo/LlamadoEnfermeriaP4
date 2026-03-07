from PIL import Image
import os
import sys

def check_img(p):
    try:
        with Image.open(p) as img:
            print(f"{p}: {img.format} {img.size} {img.mode}")
    except Exception as e:
        print(f"Error {p}: {e}")

check_img("/Users/vijugo/.gemini/antigravity/brain/a3628c68-8cb3-497c-9c6a-372b3ef9378e/hospital_startup_bg_1772818107919.png")
check_img("/Users/vijugo/.gemini/antigravity/brain/a3628c68-8cb3-497c-9c6a-372b3ef9378e/hospital_idle_dashboard_bg_1772818282789.png")

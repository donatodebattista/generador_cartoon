#!/usr/bin/env python3
"""
Generador de imágenes de prueba sintéticas con formas geométricas, degradados y texturas
para el Trabajo Práctico de Procesamiento Digital de Imágenes (Efecto Cartoon).
Resoluciones requeridas por consigna: 800x800, 2000x2000 y 5000x5000 px.
"""

import sys
import os
import math
from PIL import Image, ImageDraw

def generate_test_image(width, height, output_path):
    print(f"Generando imagen sintética de {width}x{height} en: {output_path}...")
    img = Image.new("RGB", (width, height))
    draw = ImageDraw.Draw(img)

    # 1. Fondo con degradado diagonal (cielo a tierra)
    for y in range(height):
        ratio_y = y / float(height)
        r = int(70 + 130 * (1.0 - ratio_y))
        g = int(120 + 90 * ratio_y)
        b = int(220 * (1.0 - ratio_y) + 60 * ratio_y)
        draw.line([(0, y), (width, y)], fill=(r, g, b))

    # 2. Sol brillante / círculo
    sun_radius = int(min(width, height) * 0.12)
    sun_cx = int(width * 0.25)
    sun_cy = int(height * 0.25)
    draw.ellipse([
        (sun_cx - sun_radius, sun_cy - sun_radius),
        (sun_cx + sun_radius, sun_cy + sun_radius)
    ], fill=(255, 220, 40), outline=(255, 140, 0), width=int(max(2, min(width, height) * 0.005)))

    # 3. Montañas geométricas triangulares
    mountain_color1 = (90, 80, 110)
    mountain_color2 = (60, 50, 80)
    base_y = int(height * 0.75)

    # Montaña 1 (izquierda)
    p1 = (int(width * 0.05), base_y)
    p2 = (int(width * 0.35), int(height * 0.35))
    p3 = (int(width * 0.65), base_y)
    draw.polygon([p1, p2, p3], fill=mountain_color1, outline=(40, 30, 50))

    # Montaña 2 (derecha)
    p4 = (int(width * 0.40), base_y)
    p5 = (int(width * 0.75), int(height * 0.42))
    p6 = (int(width * 1.00), base_y)
    draw.polygon([p4, p5, p6], fill=mountain_color2, outline=(30, 20, 40))

    # 4. Colinas verdes en primer plano
    hill_color = (45, 150, 60)
    draw.ellipse([
        (int(-width * 0.2), int(height * 0.6)),
        (int(width * 0.7), int(height * 1.2))
    ], fill=hill_color, outline=(25, 90, 35), width=int(max(2, min(width, height) * 0.004)))

    draw.ellipse([
        (int(width * 0.4), int(height * 0.65)),
        (int(width * 1.3), int(height * 1.3))
    ], fill=(35, 120, 50), outline=(20, 70, 30), width=int(max(2, min(width, height) * 0.004)))

    # 5. Casa / Elementos geométricos con bordes contrastantes
    house_w = int(width * 0.16)
    house_h = int(height * 0.14)
    house_x = int(width * 0.50)
    house_y = int(height * 0.68)

    # Pared
    draw.rectangle([
        (house_x, house_y),
        (house_x + house_w, house_y + house_h)
    ], fill=(230, 200, 160), outline=(100, 50, 20), width=int(max(2, min(width, height) * 0.004)))

    # Techo
    roof_p1 = (house_x - int(house_w * 0.1), house_y)
    roof_p2 = (house_x + int(house_w * 0.5), house_y - int(house_h * 0.6))
    roof_p3 = (house_x + int(house_w * 1.1), house_y)
    draw.polygon([roof_p1, roof_p2, roof_p3], fill=(200, 50, 40), outline=(120, 20, 20))

    # Puerta y ventana
    door_w = int(house_w * 0.25)
    door_h = int(house_h * 0.55)
    draw.rectangle([
        (house_x + int(house_w * 0.15), house_y + house_h - door_h),
        (house_x + int(house_w * 0.15) + door_w, house_y + house_h)
    ], fill=(80, 50, 30))

    win_size = int(house_w * 0.25)
    draw.rectangle([
        (house_x + int(house_w * 0.55), house_y + int(house_h * 0.2)),
        (house_x + int(house_w * 0.55) + win_size, house_y + int(house_h * 0.2) + win_size)
    ], fill=(135, 206, 250), outline=(50, 50, 50), width=max(1, int(win_size * 0.1)))

    # Guardar
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    img.save(output_path, quality=95)
    print(f" Imagen guardada exitosamente ({os.path.getsize(output_path) / (1024*1024):.2f} MB)")

if __name__ == "__main__":
    resolutions = [
        (800, 800, "images/input/test_800x800.png"),
        (2000, 2000, "images/input/test_2000x2000.png"),
    ]

    # Si se pasa "all" o 5000 por argumento, también generar 5000x5000
    if len(sys.argv) > 1 and ("all" in sys.argv or "5000" in sys.argv):
        resolutions.append((5000, 5000, "images/input/test_5000x5000.png"))

    for w, h, path in resolutions:
        generate_test_image(w, h, path)

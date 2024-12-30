import numpy as np
import sys

output_filename = sys.argv[1]


def mix(a, b, t):
    return a * (1 - t) + b * t


black = np.array([0, 0, 0])
red = 18 * np.array([178, 34, 34]) / 255.0
yellow = 63 * np.array([238, 230, 53]) / 255.0
white = 80 * np.array([255.0, 255.0, 255.0]) / 255.0
a = 0.1
b = 0.15
c = 0.42
d = 0.45


def emit_color(temperature):
    if temperature < a:
        return black
    if temperature < b:
        return mix(black, red, (temperature - a) / (b - a))
    if temperature < c:
        return mix(red, yellow, (temperature - b) / (c - b))
    if temperature < d:
        return mix(yellow, white, (temperature - c) / (d - c))
    return white


def pcg_fire_colors1():
    table_size = 256
    color_table = np.zeros((table_size, 3)).astype(np.float32)
    for i in range(table_size):
        temperature = i / table_size
        color_table[i] = emit_color(temperature)
    np.save(output_filename, color_table)


pcg_fire_colors1()

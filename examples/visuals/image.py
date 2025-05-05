"""
# Image visual example

Show the image visual.

"""

from pathlib import Path

import numpy as np
from PIL import Image

import datoviz as dvz


def generate_fractal(size):
    x = np.linspace(-2, 2, size)
    y = np.linspace(-2, 2, size)
    X, Y = np.meshgrid(x, y)
    Z = X + 1j * Y
    c = complex(-0.8, 0.156)
    max_iter = 200
    escape = np.zeros(Z.shape, dtype=np.float32)
    mask = np.full(Z.shape, True, dtype=bool)
    for i in range(max_iter):
        Z[mask] = Z[mask] * Z[mask] + c
        diverged = np.abs(Z) > 4.0
        escape[mask & diverged] = i
        mask &= ~diverged
    h, w = Z.shape
    return dvz.cmap('inferno', escape / max_iter).reshape((h, w, 4))


def load_image():
    ROOT_DIR = Path(__file__).resolve().parent.parent.parent
    filepath = ROOT_DIR / 'data/textures/image.png'
    with Image.open(filepath) as f:
        return np.array(f.convert('RGBA'), dtype=np.uint8)


image = load_image()
# image = generate_fractal(1024)
height, width, _ = image.shape

position = np.array([[0, 0, 0]], dtype=np.float32)
size = np.array([[width, height]], dtype=np.float32)
anchor = np.array([[0, 0]], dtype=np.float32)
texcoords = np.array([[0, 0, 1, 1]], dtype=np.float32)

app = dvz.App()
figure = app.figure()
panel = figure.panel()
panzoom = panel.panzoom()

visual = app.image(position=position, size=size, anchor=anchor, texcoords=texcoords)
texture = app.texture_2D(image)
visual.set_texture(texture)
panel.add(visual)

app.run()
app.destroy()

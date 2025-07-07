"""
# Marker visual

Show the different types of marker visuals.

---
tags:
  - marker
  - texture
  - svg
dependencies:
  - imageio
in_gallery: true
make_screenshot: true
---

"""

from pathlib import Path

import imageio.v3 as iio
import numpy as np

import datoviz as dvz

ROOT_DIR = Path(__file__).resolve().parent.parent.parent
W, H = 800, 600
HW, HH = W / 2.0, H / 2.0
svg_path = 'M50,10 L61.8,35.5 L90,42 L69,61 L75,90 L50,75 L25,90 L31,61 L10,42 L38.2,35.5 Z'


def generate_data():
    grid_x = 6
    grid_y = 5
    N = grid_x * grid_y

    # Grid coordinates in [-1, 1]
    x = np.linspace(-1, 1, grid_x)
    y = np.linspace(-1, 1, grid_y)
    X, Y = np.meshgrid(x, y)
    x_flat = X.flatten()
    y_flat = Y.flatten()
    z_flat = np.zeros_like(x_flat)

    positions = np.stack([x_flat, y_flat, z_flat], axis=1).astype(np.float32)
    positions *= 0.8  # margin

    # Hue along x-axis
    hue = (x_flat + 1) / 2
    colors = dvz.cmap('hsv', hue)

    # Size: exponential growth from 10px to 50px along y-axis
    y_norm = (y_flat + 1) / 2
    sizes = 25 * 2.0**y_norm
    sizes = sizes.astype(np.float32)

    return N, positions, colors, sizes


def load_texture_rgba(path):
    arr = iio.imread(path)
    return arr


def make_texture(image):
    assert image.ndim == 3
    assert image.shape[2] == 4
    assert image.dtype == np.uint8
    return app.texture(image)


def make_svg_msdf_texture(svg_path, size=64):
    msdf = dvz.msdf_from_svg(svg_path, size, size)
    msdf_alpha = np.empty((size, size, 4), dtype=np.float32)
    dvz.rgb_to_rgba_float(size * size, msdf, msdf_alpha.ravel())
    return app.texture(msdf_alpha)


def make_visual(panel):
    N, position, color, size = generate_data()
    angle = np.linspace(0, 2 * np.pi, N)
    visual = app.marker(
        position=position,
        color=color,
        size=size,
        angle=angle,
        edgecolor=(255, 255, 255, 255),
        linewidth=2.0,
    )
    panel.add(visual)
    return visual


app = dvz.App()
figure = app.figure()

# Code Outline
panel = figure.panel(offset=(0, 0), size=(HW, HH))
visual = make_visual(panel)
visual.set_mode('code')
visual.set_aspect('outline')
visual.set_shape('club')  # pre-defined shapes coded in the shaders

# Bitmap
panel = figure.panel(offset=(HW, 0), size=(HW, HH))
visual = make_visual(panel)
visual.set_mode('bitmap')
visual.set_aspect('filled')
visual.set_shape('club')
image = load_texture_rgba(dvz.download_data('textures/pushpin.png'))
texture = make_texture(image)
visual.set_texture(texture)  # bitmap textures

# Code Stroke
panel = figure.panel(offset=(0, HH), size=(HW, HH))
visual = make_visual(panel)
visual.set_mode('code')
visual.set_aspect('stroke')
visual.set_shape('spade')

# SVG
panel = figure.panel(offset=(HW, HH), size=(HW, HH))
visual = make_visual(panel)
visual.set_tex_scale(100)  # Important: let the visual know about the texture size
visual.set_mode('msdf')
visual.set_aspect('outline')
msdf = make_svg_msdf_texture(svg_path, size=100)
visual.set_texture(msdf)

app.run()
app.destroy()

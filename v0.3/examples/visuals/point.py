"""
# Point visual

Show the point visual.

---
tags:
  - point
  - panzoom
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz


def generate_data():
    grid_x = 16
    grid_y = 12
    N = grid_x * grid_y

    # Grid coordinates in [-1, 1]
    x = np.linspace(-1, 1, grid_x)
    y = np.linspace(-1, 1, grid_y)
    X, Y = np.meshgrid(x, y)
    x_flat = X.flatten()
    y_flat = Y.flatten()
    z_flat = np.zeros_like(x_flat)

    positions = np.stack([x_flat, y_flat, z_flat], axis=1).astype(np.float32)
    positions *= 0.90  # margin

    # Hue along x-axis
    hue = (x_flat + 1) / 2
    colors = dvz.cmap('hsv', hue)

    # Size: exponential growth from 10px to 50px along y-axis
    y_norm = (y_flat + 1) / 2
    sizes = 10 * 4.0**y_norm
    sizes = sizes.astype(np.float32)

    return N, positions, colors, sizes


N, position, color, size = generate_data()

app = dvz.App()
figure = app.figure()
panel = figure.panel()
panzoom = panel.panzoom()

visual = app.point(position=position, color=color, size=size)
panel.add(visual)

app.run()
app.destroy()

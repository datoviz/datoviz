"""
# Shapes

---
tags:
  - shape
  - colormap
  - arcball
  - mesh
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz

rows = 12
cols = 16
N = rows * cols
t = np.linspace(0, 1, N)

x, y = np.meshgrid(np.linspace(-1, 1, rows), np.linspace(-1, 1, cols))
z = np.zeros_like(x)

offsets = np.c_[x.flat, y.flat, z.flat]
scales = 1.0 / rows * (1 + 0.25 * np.sin(5 * 2 * np.pi * t))
colors = dvz.cmap('hsv', np.mod(t, 1))

sc = dvz.ShapeCollection()
for offset, scale, color in zip(offsets, scales, colors):
    sc.add_cube(offset=offset, scale=scale, color=color)

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
arcball = panel.arcball(initial=(-1, -0.1, -0.25))

visual = app.mesh(sc, lighting=True)

panel.add(visual)

app.run()
app.destroy()
sc.destroy()

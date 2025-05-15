"""
# Shapes

---
tags:
  - shape
  - colormap
  - arcball
  - mesh
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
panel = figure.panel()
arcball = panel.arcball(initial=(-1, -0.1, -0.25))

visual = app.mesh_shape(sc, lighting=True)

visual.set_light_color((  0,   0, 255), 0)    # Blue
visual.set_light_color((  0, 255,   0), 1)    # Green
visual.set_light_color((200, 200,   0), 2)    # Yellow
visual.set_light_color((255,   0,   0), 3)    # Red

visual.set_light_dir(( -9, 1, 5), 0)
visual.set_light_dir(( -3, 1, 5), 1)
visual.set_light_dir((  3, 1, 5), 2)
visual.set_light_dir((  9, 1, 5), 3)

# TODO: Change to set_material_params()  Use only one call.
# Material affect on colors      R    G    B
visual.set_light_params(( .2,  .2,  .2), 0)    # Ambient
visual.set_light_params(( .8,  .8,  .8), 1)    # Diffuse
visual.set_light_params(( .8,  .8,  .8), 2)    # Specular
visual.set_light_params(( .9,  .9,  .9), 3)    # Exponent

panel.add(visual)

app.run()
app.destroy()
sc.destroy()

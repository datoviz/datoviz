"""
# Mesh visual example

Show the mesh visual with predefined shapes.

---
tags:
  - volume
  - arcball
  - camera
  - texture
---

"""

import gzip

import numpy as np

import datoviz as dvz


def load_mouse_brain():
    filepath = dvz.download_data('volumes/allen_mouse_brain_rgba.npy.gz')
    with gzip.open(filepath, 'rb') as f:
        return np.load(f)


volume = load_mouse_brain()
shape = volume.shape
dtype = volume.dtype
D, H, W = shape[:3]
scaling = 1.0 / D
x, y, z = W * scaling, H * scaling, 1

app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball(initial=(-2.25, 0.65, 1.5))
camera = panel.camera(initial=(0, 0, 3))

visual = app.volume(mode='rgba', transfer=(0.2, 0, 0, 0))
visual.set_bounds((-x, +x), (-y, +y), (-z, +z))

texture = app.texture_3D(volume, shape=(W, H, D), interpolation='linear')
visual.set_texture(texture)
panel.add(visual)

app.run()
app.destroy()

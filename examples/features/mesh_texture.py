"""
# Textured mesh cube

Show a textured mesh using a cube and a crate texture.

---
tags:
  - mesh
  - texture
  - shape
  - arcball
dependencies:
  - imageio
in_gallery: true
make_screenshot: true
---

"""

import imageio.v3 as iio
import numpy as np

import datoviz as dvz


def load_texture_rgba(path: str) -> np.ndarray:
    image = iio.imread(path)
    if image.ndim == 2:
        image = np.repeat(image[:, :, None], 3, axis=2)
    if image.shape[2] == 3:
        h, w, _ = image.shape
        image = np.dstack((image, np.full((h, w), 255, dtype=image.dtype)))
    return image.astype(np.uint8)


texture_path = dvz.download_data('textures/crate.jpg')
image = load_texture_rgba(texture_path)

sc = dvz.ShapeCollection()
sc.add_cube()

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
panel.arcball()
panel.camera(initial=(0, 0, 3))

texture = app.texture(image, interpolation='linear', address_mode='repeat')
visual = app.mesh(sc, lighting=True, texture=texture)
panel.add(visual)
panel.gizmo()

app.run()
app.destroy()

sc.destroy()

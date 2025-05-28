"""
# Image visual

Show the image visual.

---
tags:
  - image
  - panzoom
  - texture
dependencies:
  - imageio
in_gallery: true
make_screenshot: true
---

"""

import imageio.v3 as iio
import numpy as np

import datoviz as dvz


def load_image():
    filepath = dvz.download_data('textures/image.png')
    arr = iio.imread(filepath)
    h, w, _ = arr.shape
    return np.dstack((arr, np.full((h, w), 255))).astype(np.uint8)


image = load_image()
height, width, _ = image.shape

position = np.array([[0, 0, 0]], dtype=np.float32)
size = np.array([[width, height]], dtype=np.float32)
anchor = np.array([[0, 0]], dtype=np.float32)
texcoords = np.array([[0, 0, 1, 1]], dtype=np.float32)

app = dvz.App()
figure = app.figure()
panel = figure.panel()
panzoom = panel.panzoom()

visual = app.image(
    rescale='keep_ratio',
    position=position,
    size=size,
    anchor=anchor,
    texcoords=texcoords,
    #
    # Image border
    linewidth=10,
    edgecolor=(255, 255, 255, 255),
    radius=50,
)
texture = app.texture_2D(image, interpolation='linear')  # by default, no interpolation
visual.set_texture(texture)
panel.add(visual)

app.run()
app.destroy()

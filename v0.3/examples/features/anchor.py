"""
# Image anchor

Show how to use the image anchor. The anchor is a pair (x, y) within [-1, +1]^2 that indicates,
in normalized coordinates within the image, the position within the image corresponding to the
image position specified in Image.set_position().
This normalized coordinate system within the image is centered around the center of the image,
x goes right, y goes up.

---
tags:
  - image
  - anchor
  - texture
  - gui
dependencies:
  - imageio
in_gallery: true
make_screenshot: false
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
size = np.array([[1, 1]], dtype=np.float32)
anchor = np.array([[0, 0]], dtype=np.float32)
texcoords = np.array([[0, 0, 1, 1]], dtype=np.float32)

app = dvz.App()
figure = app.figure(800, 600, gui=True)
panel = figure.panel(background=True)
panzoom = panel.panzoom()

visual = app.image(
    position=position,
    size=size,
    anchor=anchor,
    texcoords=texcoords,
    rescale='keep_ratio',
    unit='ndc',
)
texture = app.texture_2D(image, interpolation='linear')
visual.set_texture(texture)
panel.add(visual)


x_anchor = dvz.Out(0.0)
y_anchor = dvz.Out(0.0)


@app.connect(figure)
def on_gui(ev):
    dvz.gui_size(dvz.vec2(400, 100))
    dvz.gui_begin('Change the image anchor', 0)
    has_changed = False
    has_changed |= dvz.gui_slider('x anchor', -1, +1, x_anchor)
    has_changed |= dvz.gui_slider('y anchor', -1, +1, y_anchor)
    dvz.gui_end()

    if has_changed:
        anchor[0, 0] = x_anchor.value
        anchor[0, 1] = y_anchor.value
        visual.set_anchor(anchor)


app.run()
app.destroy()

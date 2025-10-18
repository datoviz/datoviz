"""
# Wiggle visual

Show the wiggle visual.

---
tags:
  - wiggle
  - panzoom
  - texture
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz


def load_data(channels, samples):
    """Create a texture with the given number of channels and samples."""
    c0 = channels / 2.0
    alpha = 2 * np.pi * 8
    beta = 1.0

    s = np.linspace(0, 1, samples)
    x = s - 0.5
    y = np.sinc(alpha * x / np.pi)

    c = np.arange(channels).reshape(-1, 1)
    gamma = np.exp(-beta * ((2 * (c - c0) / c0) ** 2))

    texdata = gamma * y
    return texdata.astype(np.float32).reshape((channels, samples))


channels = 16
samples = 1024

data = load_data(channels, samples)
height, width = data.shape

position = np.array([[0, 0, 0]], dtype=np.float32)
size = np.array([[width, height]], dtype=np.float32)
anchor = np.array([[0, 0]], dtype=np.float32)
texcoords = np.array([[0, 0, 1, 1]], dtype=np.float32)

app = dvz.App()
figure = app.figure(gui=True)
panel = figure.panel()
panzoom = panel.panzoom()

visual = app.wiggle(
    scale=1.0,
    negative_color=(128, 128, 128, 255),
    positive_color=(0, 0, 0, 255),
    edgecolor=(0, 0, 0, 255),
)
texture = app.texture_2D(data, interpolation='linear')
visual.set_texture(texture)
panel.add(visual)


scale = dvz.Out(1.0)


@app.connect(figure)
def on_gui(ev):
    dvz.gui_pos(dvz.vec2(10, 10), dvz.vec2(0, 0))
    dvz.gui_size(dvz.vec2(200, 70))
    dvz.gui_begin('Change the scale', 0)
    has_changed = False
    has_changed |= dvz.gui_slider('scale', 0.1, 5, scale)
    dvz.gui_end()

    if has_changed:
        visual.set_scale(scale.value)


app.run()
app.destroy()

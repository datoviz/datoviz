"""
# Colorbar

Show how to display a colorbar.

---
tags:
  - colorbar
in_gallery: true
make_screenshot: true
---

"""

import numpy as np
import datoviz as dvz
from datoviz import Out, vec2, vec3


app = dvz.App()
figure = app.figure(gui=True)
panel = figure.panel(background=True)
panel.panzoom()
colorbar = figure.colorbar()


slider = Out(10.0)
dropdown = Out(0)
cmaps = ['hsv', 'viridis', 'magma']


@app.connect(figure)
def on_gui(ev):
    dvz.gui_pos(vec2(25, 25), vec2(0, 0))
    dvz.gui_size(vec2(400, 100))
    dvz.gui_begin('GUI', 0)

    if dvz.gui_dropdown('Colormap', 3, cmaps, dropdown, 0):
        colorbar.set_cmap(cmaps[dropdown.value])

    if dvz.gui_slider('Range', 1.0, 100.0, slider):
        colorbar.set_range(slider.value, slider.value * 2)

    dvz.gui_end()


app.run()
app.destroy()

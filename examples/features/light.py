"""
# Light example

Show how to manipulate lights.

---
tags:
  - sphere
  - light
  - gui
---

"""
import numpy as np
import datoviz as dvz

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure(gui=True)
panel = figure.panel()

visual = panel.demo_3D()


light_pos = dvz.vec3(-5, +5, +5)
light_params = dvz.vec4(0.25, 0.5, 0.5, 0.5)

# GUI callback
def update_params():
    lp = (light_params[0], light_params[1], light_params[2], light_params[3])
    visual.set_light_pos(light_pos)
    visual.set_light_params(lp)
    visual.update()

update_params()

@app.connect(figure)
def on_gui(ev):
    dvz.gui_size(dvz.vec2(500, 100))
    dvz.gui_begin('Change the light', 0)
    has_changed = False
    has_changed |= dvz.gui_slider_vec3('light pos', -10, +10, light_pos)
    has_changed |= dvz.gui_slider_vec4('light params', 0, 1, light_params)
    dvz.gui_end()

    if has_changed:
        update_params()


app.run()
app.destroy()

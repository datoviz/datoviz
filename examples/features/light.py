"""
# Light manipulation

Show how to manipulate lights.

---
tags:
  - sphere
  - light
  - gui
in_gallery: true
make_screenshot: true
---

"""

import datoviz as dvz
from datoviz import vec4

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure(gui=True)
panel = figure.panel(background=True)

visual = panel.demo_3D()

light_pos = (
    vec4(+0, +0, +5, +1),  # Pos 0  x,y,z
    vec4(-5, +0, +5, +1),  # Pos 1  x,y,z
    vec4(+0, +5, +5, +1),  # Pos 2  x,y,z
    vec4(+5, +0, +5, +1),  # Pos 3  x,y,z
)

light_color = (
    vec4(1, 1, 1, 1),  # White
    vec4(1, 0, 0, 1),  # Red
    vec4(0, 1, 0, 1),  # Blue
    vec4(0, 0, 1, 1),  # Green
)


# GUI callback
def update_params():
    for i in range(4):
        c = light_color[i]
        visual.set_light_color(
            (int(c[0] * 255), int(c[1] * 255), int(c[2] * 255), int(c[3] * 255)), i
        )
        visual.set_light_pos(light_pos[i], i)
    visual.update()


update_params()


@app.connect(figure)
def on_gui(ev):
    dvz.gui_size(dvz.vec2(400, 350))
    dvz.gui_begin('Change the lights', 0)
    has_changed = False
    dvz.gui_text('Light 1:')
    has_changed |= dvz.gui_slider_vec4('Pos 0 XYZ', -20, +20, light_pos[0])
    has_changed |= dvz.gui_slider_vec4('Color 0 RGBA', 0, 1, light_color[0])

    dvz.gui_text('Light 2:')
    has_changed |= dvz.gui_slider_vec4('Pos 1 XYZ', -20, +20, light_pos[1])
    has_changed |= dvz.gui_slider_vec4('Color 1 RGBA', 0, 1, light_color[1])

    dvz.gui_text('Light 3:')
    has_changed |= dvz.gui_slider_vec4('Pos 2 XYZ', -20, +20, light_pos[2])
    has_changed |= dvz.gui_slider_vec4('Color 2 RGBA', 0, 1, light_color[2])

    dvz.gui_text('Light 4:')
    has_changed |= dvz.gui_slider_vec4('Pos 3 XYZ', -20, +20, light_pos[3])
    has_changed |= dvz.gui_slider_vec4('Color 3 RGBA', 0, 1, light_color[3])
    dvz.gui_end()

    if has_changed:
        update_params()


app.run()
app.destroy()

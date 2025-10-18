"""
# Toggling visual visibility

Show how to show/hide a visual.

---
tags:
  - gui
  - show
in_gallery: true
make_screenshot: false
---

"""

import datoviz as dvz
from datoviz import Out

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure(gui=True)
panel = figure.panel(background=True)
visual = panel.demo_2D()

visible = Out(True)


@app.connect(figure)
def on_gui(ev):
    dvz.gui_begin('GUI', 0)
    if dvz.gui_checkbox('Visible?', visible):
        visual.show(visible.value)
        figure.update()
    dvz.gui_end()


app.run()
app.destroy()

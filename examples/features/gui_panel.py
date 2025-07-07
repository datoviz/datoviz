"""
# GUI panels

Show how to create a GUI panel.

---
tags:
  - gui_panel
in_gallery: true
make_screenshot: false
---

"""

import datoviz as dvz

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure(gui=True)

# Create a panel, specifying the panel offset and size (x, y, width, height, in pixels).
panel1 = figure.panel((50, 50), (300, 300))
panel1.demo_3D()

panel2 = figure.panel((400, 100), (300, 300))
panel2.demo_2D()

# We transform the static panels into GUI panels (experimental).
panel1.gui('First panel')
panel2.gui('Second panel')

app.run()
app.destroy()

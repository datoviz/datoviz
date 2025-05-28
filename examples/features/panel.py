"""
# Panels

Show how to create several panels.

---
tags:
  - panel
  - clip
in_gallery: true
make_screenshot: true
---

"""

import datoviz as dvz

app = dvz.App()
figure = app.figure()

# Create two panels side-by-side.
panel1 = figure.panel((0, 0), (400, 600))
panel2 = figure.panel((400, 0), (400, 600))

# Add demo visuals to the panels.
visual1 = panel1.demo_2D()
visual2 = panel2.demo_3D()

# Set some margins for the first panel, which affects the panel's coordinate systems.
# [-1, +1] map to the "inner" viewport.
panel1.margins(20, 100, 20, 20)  # top, right, bottom, left

# Indicate that the first visual should be hidden inside the margins, outside of [-1, +1].
visual1.clip('outer')

app.run()
app.destroy()

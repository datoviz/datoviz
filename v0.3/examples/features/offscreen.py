"""
# Screenshots and offscreen rendering

Show how to render an offscreen image.

!!! warning

  The API for this feature may change in an upcoming version.

---
tags:
  - offscreen
in_gallery: true
make_screenshot: false
---

"""

import datoviz as dvz

app = dvz.App(offscreen=True)
figure = app.figure()
panel = figure.panel(background=True)
panel.demo_2D()

# Save a PNG screenshot.
app.screenshot(figure, 'offscreen_python.png')

app.destroy()

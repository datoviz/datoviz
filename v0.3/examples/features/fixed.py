"""
# Fixed dimensions on a visual

Show how to fix a visual in the panel on one or several axes.

---
tags:
  - fixed
in_gallery: true
make_screenshot: false
---

"""

import datoviz as dvz

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
visual = panel.demo_2D()
visual.fixed('y')  # or 'x', or 'z', or 'x, y'... or True for all axes

app.run()
app.destroy()

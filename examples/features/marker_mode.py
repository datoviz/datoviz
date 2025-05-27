"""
# Marker rendering modes

Show the different rendering modes of the marker visual.

---
tags:
  - marker
in_gallery: false
make_screenshot: false
---

"""

import datoviz as dvz

app = dvz.App(background='white')
figure = app.figure(128, 128)
panel = figure.panel()
visual = app.marker(
    mode='code',
    shape='asterisk',
    aspect='outline',
    position=[[0, 0, 0]],
    color=[[124, 141, 194, 255]],
    size=[64],
    edgecolor=(0, 0, 0, 255),
    linewidth=2.0,
)
panel.add(visual)

app.run()
app.destroy()

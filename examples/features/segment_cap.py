"""
# Segment caps

Show the different cap of the segment visual.

---
tags:
  - segment
  - cap
in_gallery: false
make_screenshot: false
---

"""

import numpy as np

import datoviz as dvz

initial = np.array([[-0.5, 0, 0]])
terminal = np.array([[+0.5, 0, 0]])
color = np.array([[0, 0, 0, 255]])

app = dvz.App(background='white')
figure = app.figure(128, 128)
panel = figure.panel()
visual = app.segment(
    initial=initial, terminal=terminal, color=color, linewidth=50, cap=('round', 'round')
)
panel.add(visual)

app.run()
app.destroy()

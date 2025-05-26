"""
# Segment cap

Show the different cap of the segment visual.

---
tags:
  - segment
  - cap
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
visual = app.segment()
visual.set_position(initial, terminal)
visual.set_color(color)
visual.set_linewidth(50)
visual.set_cap('round', 'round')
panel.add(visual)

app.run()
app.destroy()

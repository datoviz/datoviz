"""
# Segment visual example

Show the segment visual.

---
tags:
  - segment
  - colormap
  - cap
  - panzoom
---

"""

import numpy as np

import datoviz as dvz


def generate_data():
    N = 16
    DVZ_CAP_COUNT = dvz.CAP_COUNT

    x = np.linspace(-1, 1, N).astype(np.float32) * 0.9
    y0 = -0.5
    y1 = 0.5
    z = 0.0

    initial = np.stack([x, np.full(N, y0), np.full(N, z)], axis=1).astype(np.float32)
    terminal = np.stack([x, np.full(N, y1), np.full(N, z)], axis=1).astype(np.float32)

    linewidths = np.linspace(2, 30, N).astype(np.float32)

    t = np.linspace(0, 1, N).astype(np.float32)
    colors = dvz.cmap('hsv', t, 0.0, 1.0)

    initial_caps = np.array([(2 * i) % DVZ_CAP_COUNT for i in range(N)], dtype=np.uint8)
    terminal_caps = np.array([(2 * i + 1) % DVZ_CAP_COUNT for i in range(N)], dtype=np.uint8)

    return N, initial, terminal, colors, linewidths, initial_caps, terminal_caps


N, initial, terminal, color, linewidth, initial_cap, terminal_cap = generate_data()

app = dvz.App()
figure = app.figure()
panel = figure.panel()
panzoom = panel.panzoom()

visual = app.segment()
visual.set_position(initial, terminal)
visual.set_color(color)
visual.set_linewidth(linewidth)
visual.set_cap(initial_cap, terminal_cap)
panel.add(visual)

app.run()
app.destroy()

"""
# Segment visual

Show the segment visual.

---
tags:
  - segment
  - colormap
  - panzoom
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz


def generate_data():
    N = 16

    x = np.linspace(-1, 1, N).astype(np.float32) * 0.9
    y0 = -0.5
    y1 = 0.5
    z = 0.0

    initial = np.stack([x, np.full(N, y0), np.full(N, z)], axis=1).astype(np.float32)
    terminal = np.stack([x, np.full(N, y1), np.full(N, z)], axis=1).astype(np.float32)

    linewidths = np.linspace(2, 30, N).astype(np.float32)

    t = np.linspace(0, 1, N).astype(np.float32)
    colors = dvz.cmap('hsv', t, 0.0, 1.0)

    return N, initial, terminal, colors, linewidths


N, initial, terminal, color, linewidth = generate_data()

app = dvz.App()
figure = app.figure()
panel = figure.panel()
panzoom = panel.panzoom()

visual = app.segment(
    initial=initial,
    terminal=terminal,
    color=color,
    linewidth=linewidth,
    cap=('round', 'round'),
)
panel.add(visual)

app.run()
app.destroy()

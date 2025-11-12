"""
# Path visual

Show the path visual.

---
tags:
  - path
  - panzoom
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz


def generate_data(n_groups=20, n_samples=10_000):
    N = n_groups * n_samples

    t = np.linspace(-1, 1, n_samples)
    group_indices = np.arange(n_groups)
    y_base = np.linspace(-1, 1, n_groups)

    positions = np.zeros((N, 3), dtype=np.float32)
    linewidths = np.zeros(N, dtype=np.float32)

    for i, g in enumerate(group_indices):
        freq = 1 + 1.5 * np.exp(0.1 * i)
        phase = g * np.pi / 4
        y_offset = y_base[i]
        y = y_offset + 0.1 * np.sin(2 * np.pi * freq * (t + 1) / 2 + phase)

        start = i * n_samples
        end = (i + 1) * n_samples
        positions[start:end, 0] = t
        positions[start:end, 1] = y
        positions[start:end, 2] = 0.0

    t = np.linspace(0, n_groups - 0.25, N).astype(np.float32)
    t = np.mod(t, 1)
    colors = dvz.cmap('hsv', t, 0, 1)

    positions[:, 0] *= 0.9
    positions[:, 1] *= 0.8

    linewidths = np.linspace(0, 1, N)
    linewidths = 8 + 6 * np.sin(2 * (n_groups - 1) * np.pi * linewidths)
    linewidths = linewidths.astype(np.float32)

    return N, positions, colors, linewidths


n_groups = 20
n_samples = 1_000
N = n_groups * n_samples
N, position, color, linewidth = generate_data(n_groups=n_groups, n_samples=n_samples)
lengths = np.full(n_groups, n_samples, dtype=np.uint32)

app = dvz.App()
figure = app.figure()
panel = figure.panel()
panzoom = panel.panzoom()

visual = app.path(
    position=position, 
    groups=n_groups, 
    color=color, 
    linewidth=linewidth, 
    cap='round', 
    join='round',
)
panel.add(visual)

app.run()
app.destroy()

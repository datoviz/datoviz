"""
# Glyph visual

Show the glyph visual.

---
tags:
  - glyph
  - colormap
  - panzoom
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz

app = dvz.App()
figure = app.figure()
panel = figure.panel()
panzoom = panel.panzoom()

# Define the strings and string parameters.
strings = ['Hello world'] * 8
string_count = len(strings)
glyph_count = sum(map(len, strings))
string_pos = np.zeros((string_count, 3), dtype=np.float32)
string_pos[:, 0] = -0.8
string_pos[:, 1] = 1 - 1.8 * np.linspace(0.3, 1, string_count) ** 2
scales = np.linspace(1, 4, string_count).astype(np.float32)

# Per-glyph parameters.
colors = dvz.cmap('hsv', np.mod(np.linspace(0, 2, glyph_count), 1))

visual = app.glyph(font_size=30)
visual.set_strings(strings, string_pos=string_pos, scales=scales)
visual.set_color(colors)

panel.add(visual)
app.run()
app.destroy()

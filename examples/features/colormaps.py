"""
# List colormaps

Show all colormaps and print on the terminal the colormap under the cursor.

---
tags:
  - colormap
  - image
  - mouse
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz

colormaps = dvz.colormaps()
n = len(colormaps)
m = 256
image = np.zeros((n, m, 4), dtype=np.uint8)
t = np.linspace(0, 1, m)
for i, cmap in enumerate(colormaps):
    image[i, ...] = dvz.cmap(cmap, t)

size = np.array([[2, 2]], dtype=np.float32)
texcoords = np.array([[0, 0, 1, 1]], dtype=np.float32)

app = dvz.App()
figure = app.figure()
panel = figure.panel()

visual = app.image(unit='ndc', size=size, texcoords=texcoords, rescale=True)
texture = app.texture_2D(np.transpose(image, (1, 0, 2)).copy())
visual.set_texture(texture)
panel.add(visual)


@app.connect(figure)
def on_mouse(ev):
    action = ev.mouse_event()
    x, y = ev.pos()
    w, _ = figure.size()
    idx = np.clip(int(np.floor(n * x / w)), 0, n - 1)
    print('\r' + f'colormap: {colormaps[idx]}'.ljust(79), end='', flush=True)


app.run()
app.destroy()

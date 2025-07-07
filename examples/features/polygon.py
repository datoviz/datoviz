"""
# Polygons

---
tags:
  - mesh
  - shape
  - colormap
  - polygon
  - ortho
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz


def make_polygon(n, center, radius):
    # WARNING: watch the direction (-t) otherwise the contour_joints won't work!
    t = np.linspace(0, 2 * np.pi, n + 1) - ((np.pi / 2.0) if n != 4 else np.pi / 4)
    t = t[:-1]
    x = center[0] + radius * np.cos(-t)
    y = center[1] + radius * np.sin(-t)
    return np.c_[x, y]


# Generate the shapes.
r = 0.25
w = 0.9
shapes = []
sizes = (4, 5, 6, 8)
colors = dvz.cmap('BWR', np.linspace(0, 1, 4))
sc = dvz.ShapeCollection()
for n, x, color in zip(sizes, np.linspace(-w, w, 4), colors):
    points = make_polygon(n, (x, 0), r)
    sc.add_polygon(points, color=color, contour=True)

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
ortho = panel.ortho()

visual = app.mesh(sc, linewidth=15, edgecolor=(255, 255, 255, 200))
panel.add(visual)

app.run()
app.destroy()
sc.destroy()

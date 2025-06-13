"""
# Mesh visual

Show the mesh visual with predefined shapes.

---
tags:
  - mesh
  - shape
  - arcball
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz

N = 5
colors = dvz.cmap('spring', np.linspace(0, 1, N))
scale = 0.35

sc = dvz.ShapeCollection()
sc.add_tetrahedron(offset=(-1, 0.5, -0.5), scale=scale, color=colors[0])
sc.add_hexahedron(offset=(0, 0.5, -0.5), scale=scale, color=colors[1])
sc.add_octahedron(offset=(1, 0.5, -0.5), scale=scale, color=colors[2])
sc.add_dodecahedron(offset=(-0.5, -0.5, 0), scale=scale, color=colors[3])
sc.add_icosahedron(offset=(+0.5, -0.5, 0), scale=scale, color=colors[4])

app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball()  # initial=(-2, 0, 0))

visual = app.mesh(sc, lighting=True)

panel.add(visual)

app.run()
app.destroy()

sc.destroy()

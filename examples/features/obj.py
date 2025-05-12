"""
# Mesh visual example

Show the mesh visual with predefined shapes.

---
tags:
  - mesh
  - shape
  - obj
  - arcball
---

"""

from pathlib import Path

import datoviz as dvz

ROOT_DIR = Path(__file__).resolve().parent.parent.parent
file_path = ROOT_DIR / 'data/mesh/bunny.obj'

linewidth = 0.1
edgecolor = (0, 0, 0, 96)
light_params = (0.25, 0.75, 0.25, 16)

sc = dvz.ShapeCollection()
sc.add_obj(file_path, contour='full')

app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball(initial=(0.35, 0, 0))
camera = panel.camera(initial=(0, 0, 3))

visual = app.mesh_shape(
    sc, lighting=True, linewidth=linewidth, edgecolor=edgecolor, light_params=light_params
)
panel.add(visual)

app.run()
app.destroy()

sc.destroy()

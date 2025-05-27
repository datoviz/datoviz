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

import datoviz as dvz

file_path = dvz.download_data('mesh/bunny.obj')

linewidth = 0.1
edgecolor = (0, 0, 0, 96)

sc = dvz.ShapeCollection()
sc.add_obj(file_path, contour='full')

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
arcball = panel.arcball(initial=(0.35, 0, 0))
camera = panel.camera(initial=(0, 0, 3))

visual = app.mesh(sc, lighting=True, linewidth=linewidth, edgecolor=edgecolor)

panel.add(visual)

app.run()
app.destroy()

sc.destroy()

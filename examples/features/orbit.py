"""
# Camera orbit

Show how to easily make an animation with a camera orbit

---
tags:
  - mesh
  - shape
  - obj
  - orbit
  - grid
in_gallery: true
make_screenshot: true
---

"""

import datoviz as dvz

sc = dvz.ShapeCollection()
file_path = dvz.download_data('mesh/bunny.obj')
sc.add_obj(file_path, contour='full')
linewidth = 0.1
edgecolor = (0, 0, 0, 96)

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
camera = panel.camera(initial=(0, 1, 3))
panel.horizontal_grid(elevation=-0.62)

visual = app.mesh(sc, lighting=True, linewidth=linewidth, edgecolor=edgecolor)
panel.add(visual)

# Camera orbit with period of 10 seconds.
panel.orbit(period=20)

app.run()
app.destroy()
sc.destroy()

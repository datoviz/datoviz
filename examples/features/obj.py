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
light_params = (0.25, 0.75, 0.9, 64.0)

sc = dvz.ShapeCollection()
sc.add_obj(file_path, contour='full')

app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball(initial=(0.35, 0, 0))
#camera = panel.camera(initial=(0, 0, 3))

visual = app.mesh_shape(
    sc, lighting=True, linewidth=linewidth, edgecolor=edgecolor, light_params=light_params
)

# TODO: move light settings to panel.
visual.set_light_color((  0,   0, 255), 0)    # Blue
visual.set_light_color((  0, 255,   0), 1)    # Green
visual.set_light_color((200, 200,   0), 2)    # Yellow
visual.set_light_color((255,   0,   0), 3)    # Red

visual.set_light_dir(( -9, 1, 5), 0)
visual.set_light_dir(( -3, 1, 5), 1)
visual.set_light_dir((  3, 1, 5), 2)
visual.set_light_dir((  9, 1, 5), 3)


# TODO: Change to set_material_params()  Use only one call.
# Material affect on colors      R    G    B
visual.set_light_params(( .2,  .2,  .2), 0)    # Ambient
visual.set_light_params(( .8,  .8,  .8), 1)    # Diffuse
visual.set_light_params(( .8,  .8,  .8), 2)    # Specular
visual.set_light_params(( .9,  .9,  .9), 3)    # Exponent

panel.add(visual)

app.run()
app.destroy()

sc.destroy()

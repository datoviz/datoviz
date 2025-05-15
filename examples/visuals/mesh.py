"""
# Mesh visual example

Show the mesh visual with predefined shapes.

---
tags:
  - mesh
  - shape
  - arcball
---

"""

import numpy as np

import datoviz as dvz

N = 5
colors = dvz.cmap('spring', np.linspace(0, 1, N))
scale = 0.35

sc = dvz.ShapeCollection()
sc.add_tetrahedron(offset=(-1, 0, 0.5), scale=scale, color=colors[0])
sc.add_hexahedron(offset=(0, 0, 0.5), scale=scale, color=colors[1])
sc.add_octahedron(offset=(1, 0, 0.5), scale=scale, color=colors[2])
sc.add_dodecahedron(offset=(-0.5, 0, -0.5), scale=scale, color=colors[3])
sc.add_icosahedron(offset=(+0.5, 0, -0.5), scale=scale, color=colors[4])

app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball(initial=(-2, 0, 0))

visual = app.mesh_shape(sc, lighting=True)

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

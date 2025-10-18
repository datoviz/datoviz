"""
# Mouse brain 3D surface mesh

Show a 3D mesh.

---
tags:
  - mesh
  - arcball
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz

data = np.load(dvz.download_data('mesh/brain.npz'))
pos = data['pos']
normal = data['normal']
color = data['color']
index = data['index']
nv, ni = pos.shape[0], index.shape[0]


angles = (-2.4, -0.9, 0.1)

# -------------------------------------------------------------------------------------------------

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
arcball = panel.arcball(initial=angles)

visual = app.mesh(indexed=True, lighting=True)
visual.set_data(
    position=pos,
    normal=normal,
    color=color,
    index=index,
)
panel.add(visual)

app.run()
app.destroy()

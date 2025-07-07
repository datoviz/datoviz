"""
# Transparent meshes

Show a transparent 3D mesh.

!!! warning

    Transparency in 3D is difficult to handle correctly. The current implementation is suboptimal.
    Improved mesh transparency will be introduced in Datoviz v0.4, as it requires changes to the
    rendering backend.

---
tags:
  - mesh
  - transparency
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

index = index.reshape((-1, 3))[::-1, :].ravel()
color[:] = (255, 255, 255, 32)

angles = (-2.5, -0.9, -0.1)

# -------------------------------------------------------------------------------------------------

app = dvz.App(background='white')
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball(initial=angles)

visual = app.mesh(indexed=True, lighting=True, cull='back')
visual.set_data(
    position=pos,
    normal=normal,
    color=color,
    index=index,
)
panel.add(visual)

app.run()
app.destroy()

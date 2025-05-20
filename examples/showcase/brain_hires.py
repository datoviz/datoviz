"""
# Mesh example: high-resolution 3D mesh surface of human brain

Show a 3D mesh.

---
tags:
  - mesh
  - arcball
---

"""

from pathlib import Path

import numpy as np

import datoviz as dvz

# -------------------------------------------------------------------------------------------------
# Load VTK file
# -------------------------------------------------------------------------------------------------

CURDIR = Path(__file__).resolve().parent.parent.parent

data = np.load(CURDIR / 'data/mesh/brain_hires.npz')
pos = data['pos']
normal = data['normal']
curvature = data['curvature']
index = data['index']
nv, ni = pos.shape[0], index.shape[0]
print(f'Loaded mesh with {nv} vertices and {ni // 3} faces.')

colors = np.full((nv, 4), 255, dtype=np.uint8)

light_dir = (+1, -1, -1)
light_params = (0.5, 0.5, 0.5, 16)

# -------------------------------------------------------------------------------------------------

app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball()

visual = app.mesh(indexed=True, lighting=True)
visual.set_data(
    position=pos,
    normal=normal,
    color=colors,
    index=index,
    light_dir=light_dir,
    light_params=light_params,
)
panel.add(visual)

app.run()
app.destroy()

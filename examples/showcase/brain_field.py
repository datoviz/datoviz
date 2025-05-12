"""
# Mesh example

Show a 3D mesh.

---
tags:
  - mesh
  - arcball
---

"""

from pathlib import Path

import nibabel as nib
import numpy as np
from nilearn import datasets

import datoviz as dvz

# -------------------------------------------------------------------------------------------------
# Brain with scalar field
# -------------------------------------------------------------------------------------------------

CURDIR = Path(__file__).resolve().parent.parent.parent

fsaverage = datasets.fetch_surf_fsaverage()
left_pial = fsaverage.pial_left
coords, faces = nib.load(left_pial).darrays[0].data, nib.load(left_pial).darrays[1].data

sulc_map = nib.load(fsaverage.sulc_left).darrays[0].data

rest = datasets.fetch_surf_nki_enhanced()
texture = nib.load(rest.func_left[0]).darrays[0].data

center = np.mean(coords, axis=0)
scale = (coords.max(axis=0) - coords.min(axis=0)).max() / 2
pos = (coords - center) / scale

pos = pos.astype(np.float32)
nv = pos.shape[0]
index = faces.ravel().astype(np.uint32)
ni = index.size

normals = np.zeros_like(pos, dtype=np.float32)
dvz.compute_normals(nv, ni, pos.astype(np.float32), index.astype(np.uint32), normals)

f = sulc_map
colors = dvz.cmap('binary', f.astype(np.float32), f.min(), f.max())

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
    normal=normals,
    color=colors,
    index=index,
    light_dir=light_dir,
    light_params=light_params,
)
panel.add(visual)

app.run()
app.destroy()

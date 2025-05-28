"""
# Scalar field on 3D brain surface mesh

Show a 3D mesh with a scalar field colored with a colormap.

---
tags:
  - mesh
  - arcball
dependencies:
  - nibabel
  - nilearn
in_gallery: true
make_screenshot: true
---

"""

import nibabel as nib
import numpy as np
from nilearn import datasets

import datoviz as dvz

# -------------------------------------------------------------------------------------------------
# Brain with scalar field
# -------------------------------------------------------------------------------------------------

fsaverage = datasets.fetch_surf_fsaverage()


def process_hemisphere(pial_path, sulc_path):
    coords, faces = nib.load(pial_path).darrays[0].data, nib.load(pial_path).darrays[1].data
    sulc_map = nib.load(sulc_path).darrays[0].data

    pos = coords.astype(np.float32)
    index = faces.ravel().astype(np.uint32)

    normals = np.zeros_like(pos, dtype=np.float32)
    dvz.compute_normals(pos.shape[0], index.size, pos, index, normals)

    colors = dvz.cmap('binary', sulc_map.astype(np.float32), sulc_map.min(), sulc_map.max())

    return pos, normals, colors, index


# Process left hemisphere
left_pos, left_normals, left_colors, left_index = process_hemisphere(
    fsaverage.pial_left, fsaverage.sulc_left
)

# Process right hemisphere
right_pos, right_normals, right_colors, right_index = process_hemisphere(
    fsaverage.pial_right, fsaverage.sulc_right
)

# Combine left and right hemispheres
pos = np.vstack((left_pos, right_pos))
normals = np.vstack((left_normals, right_normals))
colors = np.vstack((left_colors, right_colors))
index_offset = left_pos.shape[0]
index = np.hstack((left_index, right_index + index_offset))

# Normalize after concatenation
center = np.mean(pos, axis=0)
scale = (pos.max(axis=0) - pos.min(axis=0)).max() / 2
pos = (pos - center) / scale

# -------------------------------------------------------------------------------------------------

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
arcball = panel.arcball(initial=(-1.5, 0.15, 1.5))
camera = panel.camera(initial=(0, 0, 3))

visual = app.mesh(indexed=True, lighting=True)
visual.set_data(
    position=pos,
    normal=normals,
    color=colors,
    index=index,
)
panel.add(visual)

app.run()
app.destroy()

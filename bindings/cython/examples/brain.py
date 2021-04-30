"""
# 3D brain mesh


"""

import numpy as np

from nilearn import datasets
from nilearn.surface import load_surf_data, load_surf_mesh, vol_to_surf
from nilearn import plotting

from datoviz import canvas, run, colormap

# Get the data:
fsaverage = datasets.fetch_surf_fsaverage()

# Left hemisphere.
mesh = load_surf_mesh(fsaverage['pial_left'])
coords, faces = mesh[0], mesh[1]
bg_data = load_surf_data(fsaverage['sulc_left'])

# Right hemisphere.
mesh = load_surf_mesh(fsaverage['pial_right'])
coords2, faces2 = mesh[0], mesh[1]
bg_data2 = load_surf_data(fsaverage['sulc_right'])

# Concatenate.
coords = np.vstack((coords, coords2))
faces = np.vstack((faces, faces2 + faces.max() + 1))
bg_data = np.concatenate((bg_data, bg_data2))

# Depth background data.
bg_data = (bg_data - bg_data.min()) / (bg_data.max() - bg_data.min())
N = bg_data.shape[0]
# HACK: uv tex coords to fetch the right colormap value. To be improved
cmap = 0
uv = np.c_[bg_data, np.ones(N) * cmap / 256.0 + .5 / 256.0]

# Plot the data:
c = canvas(show_fps=False, width=1024, height=768)
panel = c.scene().panel(controller='arcball')
visual = panel.visual('mesh', transform='auto')

visual.data('pos', coords)
visual.data('texcoords', uv)
visual.data('index', faces.ravel())

# Light parameters
light_params = np.zeros((4, 4))  # up to 4 lights
# ambient, diffuse, specular, specular exponent
light_params[0, :] = (.4, .4, .2, 64)
visual.data('light_params', light_params)

gui = c.gui("GUI")


@gui.control("slider_float", "glossy", value=.2, vmin=0, vmax=1)
def on_change(value):
    light_params[0, 2] = value
    visual.data('light_params', light_params)


run()

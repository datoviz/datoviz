from pathlib import Path
import time

import numpy as np
from allensdk.core.mouse_connectivity_cache import MouseConnectivityCache

from visky import canvas, run

# Download the mouse data.
mcc = MouseConnectivityCache(resolution=10)
structure_id = 315  # this is id for isocortex
cortex = mcc.get_structure_mesh(structure_id)
vertices, vn, triangles, tn = cortex
N = vertices.shape[0]
Nf = triangles.shape[0]
print(f"{N} vertices, {Nf} faces")

# Data normalization.
# TODO: do in Visky instead
vertices -= vertices.mean(axis=0)
c = .5 / np.abs(vertices).max(axis=0)
vertices *= c

# Create the scene.
canvas = canvas()
panel = canvas.panel(controller='arcball')
visual = panel.visual('mesh')

# Set the visual data.
visual.data('pos', vertices.astype(np.float32))
visual.data('normal', vn.astype(np.float32))
visual.data('texcoords', np.random.rand(N, 2).astype(np.float32))
# visual.data('texcoords', np.zeros((N, 2)).astype(np.float32))
visual.data('index', triangles.ravel().astype(np.uint32))


lights_params = np.zeros((4, 4), dtype=np.float32)
lights_params[0, 0] = 0.4
lights_params[0, 1] = 0.4
lights_params[0, 2] = 0.4

lights_pos = np.zeros((4, 4), dtype=np.float32)
lights_pos[0, 0] = -1
lights_pos[0, 1] = 0.5
lights_pos[0, 2] = +1

tex_coefs = np.zeros(4, dtype=np.float32)
tex_coefs[0] = 1

view_pos = np.zeros(4, dtype=np.float32)
view_pos[2] = 3

visual.data('light_params', lights_params)
visual.data('light_pos', lights_pos)
visual.data('texcoefs', tex_coefs)
visual.data('view_pos', view_pos)


run()

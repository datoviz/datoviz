from pathlib import Path
import time

import numpy as np
from allensdk.core.mouse_connectivity_cache import MouseConnectivityCache

from visky import canvas, run

# Download the mouse data.
mcc = MouseConnectivityCache(resolution=10)
structure_id = 315  # this is id for isocortex
cortex = mcc.get_structure_mesh(structure_id)
vertices, normals, triangles, tn = cortex
N = vertices.shape[0]
Nf = triangles.shape[0]
print(f"{N} vertices, {Nf} faces")

# Data normalization.
# TODO: do in Visky instead
vertices -= vertices.mean(axis=0)
c = .5 / np.abs(vertices).max()
vertices *= -c

texcoords = np.zeros((N, 2))

# Create the scene.
canvas = canvas()
panel = canvas.panel(controller='arcball')
visual = panel.visual('mesh')

# Set the visual data.
visual.data('pos', vertices.astype(np.float32))
visual.data('normal', normals.astype(np.float32))
visual.data('texcoords', texcoords.astype(np.float32))
visual.data('index', triangles.ravel().astype(np.uint32))


run()

from pathlib import Path
import time

import numpy as np
from allensdk.core.mouse_connectivity_cache import MouseConnectivityCache

from visky import canvas, run

# Download the mouse data.
mcc = MouseConnectivityCache(resolution=10)
structure_id = 315 # this is id for isocortex
cortex = mcc.get_structure_mesh(structure_id)
vertices, vn, triangles, tn = cortex
N = vertices.shape[0]

# Data normalization.
# TODO: do in Visky instead
vertices -= vertices.mean(axis=0)
c = .5 / np.abs(vertices).max(axis=0)
vertices *= c
vn *= c

# Create the scene.
canvas = canvas()
panel = canvas.panel(controller='arcball')
visual = panel.visual('marker')

# Set the visual data.
visual.data('pos', vertices.astype(np.float32))
visual.data('color', np.random.randint(
    low=50, high=255, size=(N, 4)).astype(np.uint8))
visual.data('ms', (2 * np.ones(1)).astype(np.float32))

run()

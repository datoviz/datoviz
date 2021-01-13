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
indices = triangles.ravel()
N = vertices.shape[0]
Nf = triangles.shape[0]
print(f"{N} vertices, {Nf} faces")

# Data normalization.
# TODO: do in Visky instead
vertices -= vertices.mean(axis=0)
c = .5 / np.abs(vertices).max()
vertices *= -c

# Create the scene.
canvas = canvas()
panel = canvas.panel(controller='arcball')

# Mesh.
mesh = panel.visual('mesh')
mesh.data('pos', vertices.astype(np.float32))
mesh.data('normal', normals.astype(np.float32))
mesh.data('index', indices.astype(np.uint32))

# Slice plane.
plane = panel.visual('volume_slice')
x = .5;
plane.data('pos', np.array([[0, -x, +x]], dtype=np.float32), idx=0)
plane.data('pos', np.array([[0, +x, -x]], dtype=np.float32), idx=1)
plane.data('texcoords', np.array([[0, 0, 0.5]], dtype=np.float32), idx=0)
plane.data('texcoords', np.array([[1, 1, 0.5]], dtype=np.float32), idx=1)
plane.data('colormap', np.array([[26]], dtype=np.int32))
plane.volume(np.random.randint(size=(4, 4, 4), low=0, high=255).astype(np.uint8))

run()

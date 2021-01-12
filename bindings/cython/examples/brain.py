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
vertices *= c
# normals *= c

texcoords = np.zeros((N, 2))


# N = 1000  # number of triangles
# vertices = np.zeros((N, 3))
# vertices[:, 0] = .75 * (2 * np.random.rand(N) - 1)
# vertices[:, 1] = .75 * (2 * np.random.rand(N) - 1)
# vertices[1::2, 2] = 1
# vertices[:, 2] = .25 + .5 * vertices[:, 2]
# vertices = np.repeat(vertices, 3, axis=0)
# vertices[0::3, 0] -= .05
# vertices[1::3, 0] += .05
# vertices[0::3, 1] -= .05
# vertices[1::3, 1] -= .05
# vertices[2::3, 1] += .05
# assert vertices.shape == (3 * N, 3)

# normals = np.zeros((3 * N, 3))
# assert normals.shape == (3 * N, 3)

# texcoords = np.zeros((N, 2))
# texcoords[0::2, 1] = 1.5 / 256.0
# texcoords[1::2, 1] = 3.5 / 256.0
# texcoords = np.repeat(texcoords, 3, axis=0)
# texcoords[0::3, 0] = 0.0
# texcoords[1::3, 0] = 0.5
# texcoords[2::3, 0] = 1.0
# assert texcoords.shape == (3 * N, 2)

# Create the scene.
canvas = canvas()
panel = canvas.panel(controller='arcball')
visual = panel.visual('mesh')

# Set the visual data.
visual.data('pos', vertices.astype(np.float32))
visual.data('normal', normals.astype(np.float32))
visual.data('texcoords', texcoords.astype(np.float32))
visual.data('index', triangles.ravel().astype(np.uint32))

lights_params = np.zeros((4, 4), dtype=np.float32, order='C')
lights_params[0, 0] = 0.3
lights_params[0, 1] = 0.4
lights_params[0, 2] = 0.3

lights_pos = np.zeros((4, 4), dtype=np.float32, order='C')
lights_pos[0, 0] = -2
lights_pos[0, 1] = 0.5
lights_pos[0, 2] = +2

tex_coefs = np.zeros(4, dtype=np.float32)
tex_coefs[0] = 1

view_pos = np.zeros(4, dtype=np.float32)
view_pos[2] = 3

visual.data('light_params', lights_params)
visual.data('light_pos', lights_pos)
visual.data('texcoefs', tex_coefs)
visual.data('view_pos', view_pos)


run()

from pathlib import Path
import time

import numpy as np
from allensdk.core.mouse_connectivity_cache import MouseConnectivityCache
from ibllib.atlas import AllenAtlas

from visky import canvas, run


RESOLUTION = 25


atlas = AllenAtlas(RESOLUTION)
i = np.nonzero(atlas.regions.name == 'Isocortex')[0][0]
id = atlas.regions.id[i]
mesh_color = atlas.regions.rgb[i]
print(mesh_color)



# Download the mouse data.
mcc = MouseConnectivityCache(resolution=RESOLUTION)
structure_id = id
cortex = mcc.get_structure_mesh(structure_id)

vertices, normals, triangles, tn = cortex
indices = triangles.ravel()
N = vertices.shape[0]
Nf = triangles.shape[0]
print(f"{N} vertices, {Nf} faces")


def _transpose(M):
    # return M
    x, y, z = M.T
    return np.c_[-z, -y, -x]


def _index_of(arr, lookup):
    lookup = np.asarray(lookup, dtype=np.int32)
    m = (lookup.max() if len(lookup) else 0) + 1
    tmp = np.zeros(m + 1, dtype=np.int)
    # Ensure that -1 values are kept.
    tmp[-1] = -1
    if len(lookup):
        tmp[lookup] = np.arange(len(lookup))
    return tmp[arr]



vertices = _transpose(vertices)
normals = _transpose(normals)



# Create the scene.
canvas = canvas()
panel = canvas.panel(controller='arcball')

# Mesh.
mesh = panel.visual('mesh')
mesh.data('pos', vertices)
mesh.data('normal', normals)
mesh.data('index', indices)
# mesh.data('clip', np.array([1, 0, 0, 0]).astype(np.float32))




root = Path('/home/cyrille/git/visky-data/yanliang/').resolve()

x = np.load(root / 'single_unit_x.npy')
y = np.load(root / 'single_unit_y.npy')
z = np.load(root / 'single_unit_z.npy')

fr = np.load(root / 'single_unit_rate.npy')
fr += fr.min() + 1


pos = np.c_[x, y, z]
pos_ccf = _transpose(atlas.xyz2ccf(pos, ccf_order='apdvml')[:, [2, 0, 1]])

l = atlas.get_labels(pos)
lr = _index_of(l, atlas.regions.id)
color = atlas.regions.rgb[lr]

visual = panel.visual('marker', depth_test=True)

N = x.size
color = np.hstack((color, 255 * np.ones((N, 1))))
# ms = 2 * np.ones(1)

visual.data('pos', pos_ccf)
visual.data('color', color)
ms = fr[:, 0, 0]
visual.data('ms', ms)

i = 0
def f():
    global i
    ms = 2 + 4 * np.sqrt(fr[:, i % fr.shape[1], 0])
    visual.data('ms', ms)
    i += 1
canvas.connect('timer', f, param=.05)


P = vertices
x0, y0, z0 = P.min(axis=0)
x1, y1, z1 = P.max(axis=0)
z = .5 * (z0 + z1)

volume, info = mcc.get_template_volume()
volume *= 100

# Slice plane.
plane = panel.visual('volume_slice')

# Top left, top right, bottom right, bottom left
plane.data('pos', np.array([[x0, y1, z]], dtype=np.float64), idx=0)
plane.data('pos', np.array([[x1, y1, z]], dtype=np.float64), idx=1)
plane.data('pos', np.array([[x1, y0, z]], dtype=np.float64), idx=2)
plane.data('pos', np.array([[x0, y0, z]], dtype=np.float64), idx=3)

P = atlas.ccf2xyz(cortex[0], ccf_order='apdvml')
x0, y0, z0 = P.min(axis=0)
x1, y1, z1 = P.max(axis=0)
z = .5 * (z0 + z1)

# TODO: compute volume indices as a function of the vertex positions
u0 = atlas.bc.x2i(x0) / (atlas.bc.nxyz[0] * 1.0)
u1 = atlas.bc.x2i(x1) / (atlas.bc.nxyz[0] * 1.0)
v0 = atlas.bc.y2i(y0) / (atlas.bc.nxyz[1] * 1.0)
v1 = atlas.bc.y2i(y1) / (atlas.bc.nxyz[1] * 1.0)
w = atlas.bc.z2i(z) / (atlas.bc.nxyz[2] * 1.0)
# print((u0, v0), (u1, v1), w)

# DEBUG
u0=v0=0
u1=v1=1
w=.5

plane.data('texcoords', np.array([[0.5, 0, 0]], dtype=np.float32), idx=0)
plane.data('texcoords', np.array([[0.5, 1, 0]], dtype=np.float32), idx=1)
plane.data('texcoords', np.array([[0.5, 1, 1]], dtype=np.float32), idx=2)
plane.data('texcoords', np.array([[0.5, 0, 1]], dtype=np.float32), idx=3)

# plane.data('texcoords', np.array([[w, u0, v1]], dtype=np.float32), idx=0)
# plane.data('texcoords', np.array([[w, u1, v1]], dtype=np.float32), idx=1)
# plane.data('texcoords', np.array([[w, u1, v0]], dtype=np.float32), idx=2)
# plane.data('texcoords', np.array([[w, u0, v0]], dtype=np.float32), idx=3)

plane.data('colormap', np.array([[26]], dtype=np.int32))
plane.data('transferx', np.array([[0, .1, .9, 1]], dtype=np.float32), idx=0)
plane.data('transfery', np.array([[1, 1, 1, 1]], dtype=np.float32), idx=0)
plane.data('transferx', np.array([[0, .1, .9, 1]], dtype=np.float32), idx=1)
plane.data('transfery', np.array([[1, 1, 1, 1]], dtype=np.float32), idx=1)
plane.volume(volume)

run()

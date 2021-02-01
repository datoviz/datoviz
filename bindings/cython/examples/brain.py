from pathlib import Path

from joblib import Memory
import numpy as np
from allensdk.core.mouse_connectivity_cache import MouseConnectivityCache
from ibllib.atlas import AllenAtlas

from datoviz import canvas, run


# -------------------------------------------------------------------------------------------------
# Utils
# -------------------------------------------------------------------------------------------------

def _index_of(arr, lookup):
    lookup = np.asarray(lookup, dtype=np.int32)
    m = (lookup.max() if len(lookup) else 0) + 1
    tmp = np.zeros(m + 1, dtype=np.int)
    # Ensure that -1 values are kept.
    tmp[-1] = -1
    if len(lookup):
        tmp[lookup] = np.arange(len(lookup))
    return tmp[arr]


def region_id(name):
    i = np.nonzero(atlas.regions.name == name)[0][0]
    return atlas.regions.id[i]


def region_color(name):
    i = np.nonzero(atlas.regions.name == 'Isocortex')[0][0]
    mesh_color = atlas.regions.rgb[i]
    return mesh_color


# -------------------------------------------------------------------------------------------------
# Loading functions
# -------------------------------------------------------------------------------------------------

RESOLUTION = 25
cachedir = Path(__file__).parent / '.joblib'
MEM = Memory(cachedir)


@MEM.cache
def get_atlas(res):
    return AllenAtlas(res)


atlas = get_atlas(RESOLUTION)
mcc = MouseConnectivityCache(resolution=RESOLUTION)


@MEM.cache
def load_mesh(name):
    vertices, normals, triangles, tn = mcc.get_structure_mesh(region_id(name))
    indices = triangles.ravel()
    N = vertices.shape[0]
    Nf = triangles.shape[0]
    print(f"Load mesh {name}: {N} vertices, {Nf} faces")
    return vertices, normals, indices


@MEM.cache
def load_yanliang(path):
    x = np.load(root / 'single_unit_x.npy')
    y = np.load(root / 'single_unit_y.npy')
    z = np.load(root / 'single_unit_z.npy')
    fr = np.load(root / 'single_unit_rate.npy')
    fr += fr.min() + 1
    pos = np.c_[x, y, z]
    l = atlas.get_labels(pos)
    lr = _index_of(l, atlas.regions.id)
    color = atlas.regions.rgb[lr]
    N = x.size
    color = np.hstack((color, 255 * np.ones((N, 1))))
    return pos, color, fr


@MEM.cache
def load_volume():
    volume, info = mcc.get_template_volume()
    volume *= 100
    return volume


def ccf2uvw(P):
    P = atlas.ccf2xyz(P, ccf_order='apdvml')
    x, y, z = P
    u = atlas.bc.x2i(x) / (atlas.bc.nxyz[0] * 1.0)
    v = atlas.bc.y2i(y) / (atlas.bc.nxyz[1] * 1.0)
    w = atlas.bc.z2i(z) / (atlas.bc.nxyz[2] * 1.0)
    return np.array([u, v, w])


# -------------------------------------------------------------------------------------------------
# Main script
# -------------------------------------------------------------------------------------------------
# Create the scene.
canvas = canvas()
panel = canvas.panel(controller='arcball', transpose='xbydzl')

# Load the mesh.
vertices, normals, indices = load_mesh('Isocortex')

# Display the mesh.
mesh = panel.visual('mesh')
mesh.data('pos', vertices)
mesh.data('normal', normals)
mesh.data('index', indices)
# mesh.data('clip', np.array([1, 0, 0, 0]).astype(np.float32))


# Load the neural activity data.
root = Path(__file__).parent / '../../../data/yanliang/'
pos, color, fr = load_yanliang(root)
pos_ccf = atlas.xyz2ccf(pos, ccf_order='apdvml')[:, [2, 0, 1]]
ms = fr[:, 0, 0]

# Display the neurons.
points = panel.visual('marker', depth_test=True)
points.data('pos', pos_ccf)
points.data('color', color)
points.data('ms', ms)
points.data('linewidth', np.ones(1))

# Animation.
i = 0


def f():
    global i
    ms = 2 + 4 * np.sqrt(fr[:, i % fr.shape[1], 0])
    points.data('ms', ms)
    i += 1


canvas.connect('timer', f, param=.05)


# CCF coords
P = vertices
x0, y0, z0 = P.min(axis=0)
x1, y1, z1 = P.max(axis=0)
x = .5 * (x0 + x1)
z0 -= .1 * (z1 - z0)
z1 += .1 * (z1 - z0)
y1 += (y1 - y0)

volume = load_volume()

# Slice plane.
plane = panel.visual('volume_slice')

# Top left, top right, bottom right, bottom left
# CDS: xbydzl
# CCF coords
P0, P1, P2, P3 = np.array([
    [x, y0, z1],
    [x, y0, z0],
    [x, y1, z0],
    [x, y1, z1],
])
plane.data('pos', np.atleast_2d(P0), idx=0)
plane.data('pos', np.atleast_2d(P1), idx=1)
plane.data('pos', np.atleast_2d(P2), idx=2)
plane.data('pos', np.atleast_2d(P3), idx=3)

# Top left, top right, bottom right, bottom left
plane.data('texcoords', np.atleast_2d(ccf2uvw(P0)[[2, 0, 1]]), idx=0)
plane.data('texcoords', np.atleast_2d(ccf2uvw(P1)[[2, 0, 1]]), idx=1)
plane.data('texcoords', np.atleast_2d(ccf2uvw(P2)[[2, 0, 1]]), idx=2)
plane.data('texcoords', np.atleast_2d(ccf2uvw(P3)[[2, 0, 1]]), idx=3)

a = (0, .05, 1, 1)
b = (0, 1, 1, 1)
plane.data('transferx', np.array([a]), idx=1)
plane.data('transfery', np.array([b]), idx=1)

plane.data('colormap', np.array([[26]], dtype=np.int32))
plane.volume(volume)

run()

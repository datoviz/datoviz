from pathlib import Path
import time

from joblib import Memory
import numpy as np
from allensdk.core.mouse_connectivity_cache import MouseConnectivityCache
from ibllib.atlas import AllenAtlas

from visky import canvas, run



# -------------------------------------------------------------------------------------------------
# Utils
# -------------------------------------------------------------------------------------------------

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



# -------------------------------------------------------------------------------------------------
# Main script
# -------------------------------------------------------------------------------------------------

# Create the scene.
canvas = canvas()
panel = canvas.panel(controller='arcball')

# Load the mesh.
vertices_, normals_, indices = load_mesh('Isocortex')
vertices = _transpose(vertices_)
normals = _transpose(normals_)

# Display the mesh.
mesh = panel.visual('mesh')
mesh.data('pos', vertices)
mesh.data('normal', normals)
mesh.data('index', indices)
# mesh.data('clip', np.array([1, 0, 0, 0]).astype(np.float32))


# Load the neural activity data.
root = Path(__file__).parent / '../../../data/yanliang/'
pos, color, fr = load_yanliang(root)
pos_ccf = _transpose(atlas.xyz2ccf(pos, ccf_order='apdvml')[:, [2, 0, 1]])
ms = fr[:, 0, 0]

# Display the neurons.
points = panel.visual('marker', depth_test=True)
points.data('pos', pos_ccf)
points.data('color', color)
points.data('ms', ms)

# Animation.
i = 0
def f():
    global i
    ms = 2 + 4 * np.sqrt(fr[:, i % fr.shape[1], 0])
    points.data('ms', ms)
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
plane.data('pos', np.array([[x0, y1, z]]), idx=0)
plane.data('pos', np.array([[x1, y1, z]]), idx=1)
plane.data('pos', np.array([[x1, y0, z]]), idx=2)
plane.data('pos', np.array([[x0, y0, z]]), idx=3)



P = atlas.ccf2xyz(vertices_, ccf_order='apdvml')
x0, y0, z0 = P.min(axis=0)
x1, y1, z1 = P.max(axis=0)
z = .5 * (z0 + z1)

# TODO: compute volume indices as a function of the vertex positions
u0 = atlas.bc.x2i(x0) / (atlas.bc.nxyz[0] * 1.0)
u1 = atlas.bc.x2i(x1) / (atlas.bc.nxyz[0] * 1.0)
v0 = atlas.bc.y2i(y0) / (atlas.bc.nxyz[1] * 1.0)
v1 = atlas.bc.y2i(y1) / (atlas.bc.nxyz[1] * 1.0)
w = atlas.bc.z2i(z) / (atlas.bc.nxyz[2] * 1.0)

plane.data('texcoords', np.array([[u0, v1, w]]), idx=0)
plane.data('texcoords', np.array([[u1, v1, w]]), idx=1)
plane.data('texcoords', np.array([[u1, v0, w]]), idx=2)
plane.data('texcoords', np.array([[u0, v0, w]]), idx=3)



a = (0, .1, 1, 1)
b = (0, 1, 1, 1)
plane.data('transferx', np.array([a]), idx=1)
plane.data('transfery', np.array([b]), idx=1)

plane.data('colormap', np.array([[26]], dtype=np.int32))
plane.volume(volume)

run()

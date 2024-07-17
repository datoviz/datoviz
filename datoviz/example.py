import ctypes
import __init__ as dvz
import numpy as np
from numpy.ctypeslib import as_ctypes_type as _ctype


def array_pointer(x, dtype=np.float32):
    if not isinstance(x, np.ndarray):
        return x
    x = x.astype(dtype)
    return x.ctypes.data_as(ctypes.POINTER(_ctype(dtype)))


# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure.
figure = dvz.figure(scene, 800, 600, 0)
panel = dvz.panel_default(figure)
pz = dvz.panel_panzoom(scene, panel)

# Point visual.
visual = dvz.point(batch, 0)

# Visual data allocation.
n = 10000
dvz.point_alloc(visual, n)

# Positions.
# pos = dvz.mock_pos2D(n, .25)  # C version
pos = np.random.normal(size=(n, 3), scale=.25)  # NumPy version
dvz.point_position(visual, 0, n, array_pointer(pos), 0)
# dvz.free(array_pointer(pos))  # only for C version, DO NOT call on NumPy version

# Colors.
color = np.random.uniform(size=(n, 4), low=50, high=240)
dvz.point_color(visual, 0, n, array_pointer(color, np.uint8), 0)

# Sizes.
size = np.random.uniform(size=(n,), low=25, high=50)
dvz.point_size(visual, 0, n, array_pointer(size), 0)

# Add the visual.
dvz.panel_visual(panel, visual)

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)

"""# Scatter plot example

This is a scatter plot example.

"""

import ctypes
from ctypes import POINTER as P_

import numpy as np

import datoviz as dvz


@ctypes.CFUNCTYPE(None, P_(dvz.DvzApp), dvz.DvzId, dvz.DvzMouseEvent)
def onmouse(app, fid, ev):
    print(ev.pos[0], ev.pos[1])


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
dvz.point_position(visual, 0, n, dvz.array_pointer(pos), 0)
# dvz.free(dvz.array_pointer(pos))  # only for C version, DO NOT call on NumPy version

# Colors.
color = np.random.uniform(size=(n, 4), low=50, high=240)
dvz.point_color(visual, 0, n, dvz.array_pointer(color, np.uint8), 0)

# Sizes.
size = np.random.uniform(size=(n,), low=25, high=50)
dvz.point_size(visual, 0, n, dvz.array_pointer(size), 0)

# Add the visual.
dvz.panel_visual(panel, visual)

# Callback.
dvz.app_onmouse(app, onmouse, None)

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)

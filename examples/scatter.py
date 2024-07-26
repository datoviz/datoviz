"""# Scatter plot example

This is a scatter plot example.

"""

import numpy as np
import datoviz as dvz
from datoviz import (
    S_,  # Python string to ctypes char*
    vec2,  # Python tuple to ctypes vec2
)


@dvz.mouse
def onmouse(app, fid, ev):
    print(ev.pos[0], ev.pos[1])


slider_value = np.array([3.14], dtype=np.float32)


@dvz.gui
def ongui(app, fid, ev):
    dvz.gui_size(vec2(250.0, 80.0))
    dvz.gui_begin(S_("Hello world"), 0)
    if dvz.gui_slider(S_("Some slider"), 0, 10, slider_value):
        print(f"The value is: {slider_value[0]:.2f}.")
    dvz.gui_end()


# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure.
figure = dvz.figure(scene, 800, 600, dvz.DvzCanvasFlags.DVZ_CANVAS_FLAGS_IMGUI)
panel = dvz.panel_default(figure)
pz = dvz.panel_panzoom(scene, panel)

# Point visual.
visual = dvz.point(batch, 0)

# Visual data allocation.
n = 100_000
dvz.point_alloc(visual, n)

# Positions.
pos = np.random.normal(size=(n, 3), scale=.25).astype(np.float32)
dvz.point_position(visual, 0, n, pos, 0)

# Colors.
color = np.random.uniform(size=(n, 4), low=50, high=240).astype(np.uint8)
dvz.point_color(visual, 0, n, color, 0)

# Sizes.
size = np.random.uniform(size=(n,), low=10, high=30).astype(np.float32)
dvz.point_size(visual, 0, n, size, 0)

# Add the visual.
dvz.panel_visual(panel, visual)

# Callback.
# dvz.app_onmouse(app, onmouse, None)

# GUI.
dvz.app_gui(app, dvz.figure_id(figure), ongui, None)

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)

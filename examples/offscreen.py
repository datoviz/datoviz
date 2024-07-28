"""# Offscreen example

This is an example showing how to generate an offscreen image and save it as a PNG.


Illustrates:

- Creating a figure, panel
- Panzoom interactivity
- Point visual
- Offscreen rendering (save to a PNG image)

"""

import numpy as np

import datoviz as dvz
from datoviz import (
    S_,  # Python string to ctypes char*
)


# Boilerplate.
app = dvz.app(dvz.APP_FLAGS_OFFSCREEN)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure.
figure = dvz.figure(scene, 800, 600, 0)
panel = dvz.panel_default(figure)
pz = dvz.panel_panzoom(panel)

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
dvz.panel_visual(panel, visual, 0)

# Run the application.
dvz.scene_run(scene, app, 0)

# Screenshot to ./offscreen.png.
dvz.app_screenshot(app, dvz.figure_id(figure), S_("offscreen_python.png"))

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)

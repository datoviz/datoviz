"""# Scatter plot example

Show points in 2D with various colors and sizes.

"""

import numpy as np
import datoviz as dvz

# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure 800x600.
figure = dvz.figure(scene, 800, 600, 0)

# Panel spanning the entire window.
panel = dvz.panel_default(figure)

# Panzoom interactivity.
pz = dvz.panel_panzoom(scene, panel)

# Point visual.
visual = dvz.point(batch, 0)

# Visual data allocation.
n = 100_000
dvz.point_alloc(visual, n)

# Point positions.
pos = np.random.normal(size=(n, 3), scale=.25).astype(np.float32)
dvz.point_position(visual, 0, n, pos, 0)

# Point colors.
color = np.random.uniform(size=(n, 4), low=50, high=240).astype(np.uint8)
dvz.point_color(visual, 0, n, color, 0)

# Point sizes.
size = np.random.uniform(size=(n,), low=10, high=30).astype(np.float32)
dvz.point_size(visual, 0, n, size, 0)

# Add the visual.
dvz.panel_visual(panel, visual)

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)

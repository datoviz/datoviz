"""# Path offscreen example

This path example illustrates how to generate an offscreen image and save it as a PNG.

Illustrates:

- Creating a figure, panel
- Panzoom interactivity
- Path visual
- Offscreen rendering (save to a PNG image)

"""

import numpy as np
import datoviz as dvz

offscreen = True

# Boilerplate.
app = dvz.app(dvz.APP_FLAGS_OFFSCREEN if offscreen else 0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure.
figure = dvz.figure(scene, 400, 800, 0)
panel = dvz.panel_default(figure)

# Panzoom interactivity.
pz = dvz.panel_panzoom(panel)

# Path visual.
visual = dvz.path(batch, 0)

# Multiple paths.
n_paths = 100
path_size = 1000
n = n_paths * path_size
path_lengths = np.full(n_paths, path_size, dtype=np.uint32)
dvz.path_alloc(visual, n)

# Positions.
x = np.linspace(-1, +1, path_size)
x = np.tile(x, (n_paths, 1))
w = np.random.uniform(size=(n_paths, 1), low=20, high=100)
d = 0.5 / (n_paths - 1)
y = d * np.sin(w * x)
y += np.linspace(-1, 1, n_paths).reshape((-1, 1))
z = np.zeros((n_paths, path_size))
pos = np.c_[x.flat, y.flat, z.flat].astype(np.float32)
dvz.path_position(visual, n, pos, n_paths, path_lengths, 0)

# Colors.
t = np.linspace(0, 1, n_paths).astype(np.float32)
color = np.full((n_paths, 4), 255, dtype=np.uint8)
dvz.colormap_array(dvz.CMAP_HSV, n_paths, t, 0, 1, color)
color = np.repeat(color, path_size, axis=0)
dvz.path_color(visual, 0, n, color, 0)

# Line width.
dvz.path_linewidth(visual, 0, n, np.full(n, 3.0, dtype=np.float32), 0)

# Add the visual.
dvz.panel_visual(panel, visual, 0)

# Run the application.
dvz.scene_run(scene, app, 0)

# Screenshot to ./offscreen.png.
if offscreen:
    dvz.app_screenshot(app, dvz.figure_id(figure), "offscreen_python.png")

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)

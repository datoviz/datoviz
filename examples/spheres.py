"""# Spheres example

Show fake 3D spheres with manual camera control.

"""

import numpy as np
import datoviz as dvz
from datoviz import vec3

# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure.
figure = dvz.figure(scene, 1000, 1000, 0)

# Panel spanning the entire window.
panel = dvz.panel_default(figure)

# Panzoom interactivity.
# camera = dvz.panel_camera(panel)
dvz.panel_arcball(panel)

# Sphere visual.
visual = dvz.fake_sphere(batch, 0)

# Visual data allocation.
n = 1_000
dvz.fake_sphere_alloc(visual, n)

# Sphere positions.
pos = np.random.normal(size=(n, 3), scale=.25).astype(np.float32)
dvz.fake_sphere_position(visual, 0, n, pos, 0)

# Sphere colors.
color = np.random.uniform(size=(n, 4), low=50, high=200).astype(np.uint8)
color[:, 3] = 255
dvz.fake_sphere_color(visual, 0, n, color, 0)

# Sphere sizes.
size = np.random.uniform(size=(n,), low=50, high=100).astype(np.float32)
dvz.fake_sphere_size(visual, 0, n, size, 0)

dvz.fake_sphere_light_pos(visual, vec3(-1, +1, +10))

# Add the visual.
dvz.panel_visual(panel, visual)

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)

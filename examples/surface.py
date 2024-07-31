"""# Surface example

Show a rotating surface in 3D.

Illustrates:

- Surface shape
- Mesh visual and surface mesh
- Arcball interactivity
- Initial arcball angles
- Manual arcball parameter update
- Timers and timer callbacks

"""

import numpy as np
import datoviz as dvz
from datoviz import vec3, vec4

# Boilerplate.
app = dvz.app(dvz.APP_FLAGS_WHITE_BACKGROUND)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure 800x600.
figure = dvz.figure(scene, 800, 600, 0)

# Panel spanning the entire window.
panel = dvz.panel_default(figure)

# Arcball interactivity.
arcball = dvz.panel_arcball(panel)

# Grid parameters.
row_count = 250
col_count = row_count
n = row_count * col_count
o = vec3(-1, 0, -1)
u = vec3(2.0 / (row_count - 1), 0, 0)
v = vec3(0, 0, 2.0 / (col_count - 1))

# Allocate heights and colors arrays.
grid = np.meshgrid(row_count, col_count)
shape = (row_count, col_count)
heights = np.zeros(shape, dtype=np.float32)

# Create grid of coordinates
x = np.arange(col_count)
y = np.arange(row_count)
xv, yv = np.meshgrid(x, y)

# Distances.
center_x = col_count / 2
center_y = row_count / 2
d = np.sqrt((xv - center_x) ** 2 + (yv - center_y) ** 2)

# Heights.
a = 4.0 * 2 * np.pi / row_count
b = 3.0 * 2 * np.pi / col_count
c = .5
hmin = -.5
hmax = +.5
heights = np.exp(-.0001 * d ** 2) * np.sin(a*xv) * np.cos(b*yv)
heights = heights.ravel().astype(np.float32)

# Colors.
colors = np.empty((n, 4), dtype=np.uint8)
dvz.colormap_array(
    dvz.CMAP_PLASMA, n, -heights, -hmax, -hmin, colors)

# Create the surface shape.
shape = dvz.shape_surface(row_count, col_count, heights, colors, o, u, v, 0)

# Create the mesh visual from the surface shape.
flags = dvz.MESH_FLAGS_LIGHTING
visual = dvz.mesh_shape(batch, shape, flags)

# Lighting parameters.
dvz.mesh_light_pos(visual, vec3(-1, +1, +10))
dvz.mesh_light_params(visual, vec4(.5, .5, .5, 16))

# Add the visual to the panel.
dvz.panel_visual(panel, visual, 0)

# Initial arcball angles.
angle = -0.39686
dvz.arcball_initial(arcball, vec3(0.42339, angle, -0.00554))
dvz.panel_update(panel)


# Timer callback: update the arcball angles in real time.
@dvz.timer
def _on_timer(app, window_id, ev):
    global angle
    angle += .01
    dvz.arcball_set(arcball, vec3(0.42339, angle, -0.00554))
    dvz.panel_update(panel)


# Create a timer (60 events per second).
dvz.app_timer(app, 0, 1. / 60., 0)

# Register a timer callback.
dvz.app_ontimer(app, _on_timer, None)

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)

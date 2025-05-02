"""# Mesh example

Show a 3D mesh.

Illustrates:

- Creating a figure, panel
- Arcball interactivity
- Loading a .OBJ mesh file
- 3D shape
- Mesh visual and shape mesh
- Colormaps
- Manual mesh colors
- Timer events
- Dynamic visual updates

"""

from pathlib import Path
import numpy as np
import datoviz as dvz
from datoviz import vec3

# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure 800x600.
figure = dvz.figure(scene, 800, 600, 0)

# Panel spanning the entire window.
panel = dvz.panel_default(figure)

# Arcball interactivity.
arcball = dvz.panel_arcball(panel)

# Load a .OBJ mesh file.
CURDIR = Path(__file__).parent
filepath = (CURDIR / "../data/mesh/brain.obj").resolve()
shape = dvz.shape()
dvz.shape_obj(shape, filepath)

# Fill artificial colors.
nv = shape.vertex_count
ni = shape.index_count
print(f"Loaded {filepath} with {nv} vertices and {ni // 3} faces.")

# Create the mesh visual from the surface shape.
flags = dvz.MESH_FLAGS_LIGHTING
visual = dvz.mesh_shape(batch, shape, flags)

# Set artificial vertex colors.
t = np.linspace(0, 1, nv).astype(np.float32)
colors = dvz.cmap(dvz.CMAP_COOLWARM, t)
dvz.mesh_color(visual, 0, nv, colors, 0)

# Add the visual to the panel.
dvz.panel_visual(panel, visual, 0)

# Initial arcball angles.
dvz.arcball_initial(arcball, vec3(+0.6, -1.2, +3.0))
dvz.panel_update(panel)


# Timer callback: update the arcball angles in real time.
@dvz.on_timer
def _on_timer(app, window_id, ev):
    a = 20 * (ev.time % 1)
    u = 1 / (1 + np.exp(-a * (t - 0.5)))

    colors = dvz.cmap(dvz.CMAP_COOLWARM, u)
    dvz.mesh_color(visual, 0, nv, colors, 0)


# Create a timer (60 events per second).
dvz.app_timer(app, 0, 1.0 / 60.0, 0)

# Register a timer callback.
dvz.app_on_timer(app, _on_timer, None)


# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.shape_destroy(shape)
dvz.scene_destroy(scene)
dvz.app_destroy(app)

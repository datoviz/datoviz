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
from datoviz import vec3, vec4, S_

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
shape = dvz.shape_obj(S_(filepath))

# Fill artificial colors.
nv = shape.vertex_count
ni = shape.index_count
print(f"Loaded {filepath} with {nv} vertices and {ni // 3} faces.")

# Create the mesh visual from the surface shape.
flags = dvz.MESH_FLAGS_LIGHTING
visual = dvz.mesh_shape(batch, shape, flags)

# Set artificial vertex colors.
t = np.linspace(0, 1, nv).astype(np.float32)
colors = np.empty((nv, 4), dtype=np.uint8)
dvz.colormap_array(dvz.CMAP_COOLWARM, nv, t, 0, 1, colors)
dvz.mesh_color(visual, 0, nv, colors, 0)

# Lighting parameters.
dvz.mesh_light_pos(visual, vec3(-1, +1, +10))
dvz.mesh_light_params(visual, vec4(.5, .5, .5, 16))

# Add the visual to the panel.
dvz.panel_visual(panel, visual, 0)

# Initial arcball angles.
dvz.arcball_initial(arcball, vec3(+0.6, -1.2, +3.0))
dvz.panel_update(panel)


# Timer callback: update the arcball angles in real time.
@dvz.timer
def _on_timer(app, window_id, ev):
    a = 20 * (ev.time % 1)
    u = 1 / (1 + np.exp(-a * (t - 0.5)))

    dvz.colormap_array(dvz.CMAP_COOLWARM, nv, u.astype(np.float32), 0, 1, colors)
    dvz.mesh_color(visual, 0, nv, colors, 0)


# Create a timer (60 events per second).
dvz.app_timer(app, 0, 1. / 60., 0)

# Register a timer callback.
dvz.app_ontimer(app, _on_timer, None)


# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)

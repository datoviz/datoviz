"""# GUI example

Display a simple GUI to control the size of a mesh.

Illustrates:

- Creating a figure, panel
- Panzoom interactivity
- Shape
- Mesh visual and shape mesh
- GUI callback
- GUI dialog
- GUI buttons
- Shape transforms
- Dynamic shape and mesh update

*Note*: the screenshot does not show the GUI at the moment, this will be fixed soon.

"""

import numpy as np
import datoviz as dvz
from datoviz import vec2, vec3


# GUI callback function.
@dvz.gui
def ongui(app, fid, ev):
    # Set the size of the next GUI dialog.
    dvz.gui_size(vec2(170, 110))

    # Start a GUI dialog with a dialog title.
    dvz.gui_begin("My GUI", 0)

    # Add two buttons. The functions return whether the button was pressed.
    incr = dvz.gui_button("Increase", 150, 30)
    decr = dvz.gui_button("Decrease", 150, 30)

    # Scaling factor.
    scale = 1.0
    if incr:
        scale = 1.1
    elif decr:
        scale = 0.9
    if incr or decr:

        # Start recording shape transforms for all vertices in the shape (first=0, count=0=all).
        dvz.shape_begin(shape, 0, 0)

        # Scaling transform.
        dvz.shape_scale(shape, vec3(scale, scale, scale))

        # Stop recording the shape transforms.
        dvz.shape_end(shape)

        # Update the mesh visual data with the new shape's data.
        dvz.mesh_reshape(visual, shape)

    # End the GUI dialog.
    dvz.gui_end()


# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure.
# NOTE: to use a GUI, use this flag. Don't use it if there is no GUI.
figure = dvz.figure(scene, 800, 800, dvz.CANVAS_FLAGS_IMGUI)
panel = dvz.panel_default(figure)
arcball = dvz.panel_arcball(panel)

# Cube colors.
colors = np.array([
    [255, 0, 0, 255],
    [0, 255, 0, 255],
    [0, 0, 255, 255],
    [255, 255, 0, 255],
    [255, 0, 255, 255],
    [0, 255, 255, 255],
], dtype=np.uint8)
shape = dvz.shape_cube(colors)

# Create a mesh visual directly instantiated with the shape data.
visual = dvz.mesh_shape(batch, shape, dvz.MESH_FLAGS_LIGHTING)

# Add the visual to the panel.
dvz.panel_visual(panel, visual, 0)

# Associate a GUI callback function with a figure.
dvz.app_gui(app, dvz.figure_id(figure), ongui, None)

# Initial arcball angles.
dvz.arcball_initial(arcball, vec3(+0.6, -1.2, +3.0))
dvz.panel_update(panel)

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.shape_destroy(shape)
dvz.scene_destroy(scene)
dvz.app_destroy(app)

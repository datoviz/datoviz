"""# GUI example

Display a simple GUI to control the size of a disc.

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

"""

import numpy as np
import datoviz as dvz
from datoviz import (
    S_,  # Python string to ctypes char*
    vec2,
    vec3,
)


# GUI callback function.
@dvz.gui
def ongui(app, fid, ev):
    # Set the size of the next GUI dialog.
    dvz.gui_size(vec2(170, 110))

    # Start a GUI dialog with a dialog title.
    dvz.gui_begin(S_("My GUI"), 0)

    # Add two buttons. The functions return whether the button was pressed.
    incr = dvz.gui_button(S_("Increase"), 150, 30)
    decr = dvz.gui_button(S_("Decrease"), 150, 30)

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

        # Update the visual after its data has changed.
        dvz.visual_update(visual)

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
pz = dvz.panel_panzoom(panel)

# Create a square shape.
color = dvz.cvec4(64, 128, 255, 255)
shape = dvz.shape_square(color)

# Create a mesh visual directly instantiated with the shape data.
visual = dvz.mesh_shape(batch, shape, 0)

# Add the visual to the panel.
dvz.panel_visual(panel, visual, 0)

# Associate a GUI callback function with a figure.
dvz.app_gui(app, dvz.figure_id(figure), ongui, None)

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.shape_destroy(shape)
dvz.scene_destroy(scene)
dvz.app_destroy(app)

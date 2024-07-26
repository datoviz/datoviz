"""# GUI example

Display a simple GUI to control the size of a disc.

"""

import numpy as np
import datoviz as dvz
from datoviz import (
    S_,  # Python string to ctypes char*
    vec2,  # Python tuple to ctypes vec2
)


square_size = np.array([1.0], dtype=np.float32)


@dvz.gui
def ongui(app, fid, ev):
    dvz.gui_size(vec2(250.0, 80.0))
    dvz.gui_begin(S_("My GUI"), 0)
    # if dvz.gui_slider(S_("Square size"), 0, 10, square_size):
    #     dvz.shape_scale(shape, square_size)
    #     dvz.mesh_reshape(visual, shape)
    dvz.gui_end()


# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure.
figure = dvz.figure(scene, 800, 800, dvz.DvzCanvasFlags.DVZ_CANVAS_FLAGS_IMGUI)
panel = dvz.panel_default(figure)
pz = dvz.panel_panzoom(scene, panel)

# Shape.
color = dvz.cvec4(64, 128, 255, 255)
shape = dvz.shape_square(color)

# Mesh visual.
visual = dvz.mesh_shape(batch, shape, 0)

# Add the visual.
dvz.panel_visual(panel, visual)

# GUI.
dvz.app_gui(app, dvz.figure_id(figure), ongui, None)

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.shape_destroy(shape)
dvz.scene_destroy(scene)
dvz.app_destroy(app)

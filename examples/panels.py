"""# Panels example

Show visuals in two different panels.

Illustrates:

- Creating a figure, panel
- Point visual
- Marker visual
- Multiple panels
- Mixing 2D and 3D in the same window
- GUI checkbox
- Show/hide a visual

"""

import ctypes
import numpy as np
import datoviz as dvz
from datoviz import vec2, Out


# -------------------------------------------------------------------------------------------------
# 1. Creating the scene
# -------------------------------------------------------------------------------------------------

# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure 800x600.
w, h = 800, 600
figure = dvz.figure(scene, w, h, dvz.CANVAS_FLAGS_IMGUI)


# -------------------------------------------------------------------------------------------------
# 2. First visual
# -------------------------------------------------------------------------------------------------

# Point visual.
visual0 = dvz.point(batch, 0)

# Visual data allocation.
n = 10_000
dvz.point_alloc(visual0, n)

# Point positions.
pos = np.random.normal(size=(n, 3), scale=0.25).astype(np.float32)
dvz.point_position(visual0, 0, n, pos, 0)

# Point colors.
color = np.random.uniform(size=(n, 4), low=50, high=240).astype(np.uint8)
color[:, 3] = 240
dvz.point_color(visual0, 0, n, color, 0)

# Point sizes.
size = np.random.uniform(size=(n,), low=10, high=30).astype(np.float32)
dvz.point_size(visual0, 0, n, size, 0)

dvz.visual_depth(visual0, dvz.DEPTH_TEST_ENABLE)


# -------------------------------------------------------------------------------------------------
# 3. Second visual
# -------------------------------------------------------------------------------------------------

# Point visual.
visual1 = dvz.marker(batch, 0)

# Visual data allocation.
n = 1_000
dvz.marker_alloc(visual1, n)

# Marker positions.
pos = np.random.normal(size=(n, 3), scale=0.25).astype(np.float32)
dvz.marker_position(visual1, 0, n, pos, 0)

# Marker colors.
color = np.random.uniform(size=(n, 4), low=50, high=240).astype(np.uint8)
color[:, 3] = 240
dvz.marker_color(visual1, 0, n, color, 0)

# Marker sizes.
size = np.random.uniform(size=(n,), low=30, high=60).astype(np.float32)
dvz.marker_size(visual1, 0, n, size, 0)

# Marker parameters.
dvz.marker_aspect(visual1, dvz.MARKER_ASPECT_OUTLINE)
dvz.marker_shape(visual1, dvz.MARKER_SHAPE_CROSS)
# dvz.marker_edgecolor(visual1, cvec4(255, 255, 255, 255))
# dvz.marker_linewidth(visual1, 3.0)


# -------------------------------------------------------------------------------------------------
# 4. Panels
# -------------------------------------------------------------------------------------------------

# Panels.
panel0 = dvz.panel(figure, 0, 0, w / 2, h)
panel1 = dvz.panel(figure, w / 2, 0, w / 2, h)

dvz.panel_arcball(panel0)
dvz.panel_panzoom(panel1)

dvz.panel_visual(panel0, visual0, 0)
dvz.panel_visual(panel1, visual1, 0)


# -------------------------------------------------------------------------------------------------
# 5. GUI with checkbox
# -------------------------------------------------------------------------------------------------

# There are four steps to add a GUI with a checkbox.
# i.    Initialize the figure with the flag `dvz.CANVAS_FLAGS_IMGUI``
# ii.   Define a global-scoped object representing the variable to be updated by the GUI.
# iii.  Define the GUI callback.
# iv.   Call `dvz.app_gui(...)`

# A wrapped boolean value with initial value False.
checked = Out(True)


@dvz.gui
def ongui(app, fid, ev):
    """GUI callback function."""

    # Set the size of the next GUI dialog.
    dvz.gui_size(vec2(170, 110))

    # Start a GUI dialog with a dialog title.
    dvz.gui_begin("My GUI", 0)

    # Add a checkbox
    # Return True if the checkbox's state has changed.
    if dvz.gui_checkbox("Show visual", checked):
        is_checked = checked.value  # Python variable with the checkbox's state

        # Show/hide the visual.
        dvz.visual_show(visual0, is_checked)

        # Update the figure after its composition has changed.
        dvz.figure_update(figure)

    # End the GUI dialog.
    dvz.gui_end()


# Associate a GUI callback function with a figure.
dvz.app_gui(app, dvz.figure_id(figure), ongui, None)


# -------------------------------------------------------------------------------------------------
# 6. Run and cleanup
# -------------------------------------------------------------------------------------------------

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)

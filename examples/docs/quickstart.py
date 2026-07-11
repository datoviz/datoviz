#!/usr/bin/env python3
"""Minimal public-API scatter plot used by the Quickstart documentation."""

import os

import numpy as np
import datoviz as dvz


# Each point is described by three arrays with the same length.
# - pos: one x/y/z position per point. z is 0, so this is a 2D scatter plot.
# - color: one red/green/blue/alpha color per point, stored as 8-bit RGBA values.
# - diameters: one point size per point, measured in screen pixels.
N = 10_000
pos = np.random.uniform(-1, 1, (N, 3)).astype(np.float32)
pos[:, 2] = 0.0
color = np.random.randint(0, 256, (N, 4), dtype=np.uint8)
color[:, 3] = 255
diameters = np.full(N, 5.0, dtype=np.float32)

# Create the scene structure: one scene, one figure, and one full-size panel.
# The scene contains the visualization, the figure has a pixel size, and the
# panel is the drawing area where the scatter plot will appear.
scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)

# Add mouse interaction to the panel. Pan/zoom is limited to X and Y because
# the points are flat, with z = 0.
controller = dvz.dvz_panzoom(scene, None)
dvz.dvz_panel_bind_controller(panel, controller, dvz.DvzDimMaskFlag.DVZ_DIM_MASK_XY)

# Create one point visual for the whole dataset. The three calls to
# dvz_visual_set_data() attach the arrays to named visual attributes.
visual = dvz.dvz_point(scene, 0)
dvz.dvz_visual_set_data(visual, "position", pos)
dvz.dvz_visual_set_data(visual, "color", color)
dvz.dvz_visual_set_data(visual, "diameter_px", diameters)

# Uploading arrays is not enough by itself: the visual must be added to a
# panel before it becomes part of the figure.
dvz.dvz_panel_add_visual(panel, visual, None)

# Open a window. In a regular Python script this blocks until the user closes
# the window. The test mode closes the nonblocking session after initialization.
session = dvz.run(
    scene,
    figure,
    title="Scatter plot",
    blocking=False if os.environ.get("DVZ_QUICKSTART_TEST") else None,
)
if session is not None and os.environ.get("DVZ_QUICKSTART_TEST"):
    session.close()

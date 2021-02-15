"""
# Quickstart tutorial with Python

"""

import time

import numpy as np
import numpy.random as nr

# Import the library.
from datoviz import canvas, run, colormap

# Create a new canvas and scene. There's only 1 subplot (panel) by default.
c = canvas(show_fps=True)

# Get a panel (by default, the one spanning the entire canvas)
# We specify the type of controller we want. Here, we want 2D axes.
panel = c.panel(controller='axes')

# We create a new "marker" visual.
visual = panel.visual('marker')

# We prepare the visual properties. Here, we set the marker positions, colors (RGBA bytes),
# and size (in pixels).
N = 100_000
pos = nr.randn(N, 3)
ms = nr.uniform(low=2, high=35, size=N)
color_values = nr.rand(N)

# Use a built-in colormap
color = colormap(color_values, vmin=0, vmax=1, alpha=.75 * np.ones(N), cmap='viridis')

# Set the visual props.
visual.data('pos', pos)
visual.data('color', color)
visual.data('ms', ms)

# We define an event callback to implement mouse picking
@c.connect
def on_mouse_click(x, y, button, modifiers=()):
    # x, y are in pixel coordinates
    # First, we find the picked panel
    p = c.panel_at(x, y)
    if not p:
        return
    # Then, we transform into the data coordinate system
    # Supported coordinate systems:
    #   target_cds='data' / 'scene' / 'vulkan' / 'framebuffer' / 'window'
    xd, yd = p.pick(x, y)
    print(f"Pick at ({xd:.4f}, {yd:.4f}), modifiers={modifiers}")

# We create a GUI dialog.
gui = c.gui("Test GUI")

# We add a control, a slider controlling a float
@gui.control("slider_float", "marker size", vmin=.5, vmax=2)
def on_change(value):
    # Every time the slider value changes, we update the visual's marker size
    visual.data('ms', ms * value)
    # NOTE: an upcoming version will support partial updates

# We add another control, a slider controlling an int between 1 and 4, to change the colormap.
# NOTE: an upcoming version will provide a dropdown menu control
cmaps = ['viridis', 'cividis', 'autumn', 'winter']

@gui.control("slider_int", "colormap", vmin=0, vmax=3)
def on_change(value):
    # Recompute the colors.
    color = colormap(
        color_values, vmin=0, vmax=1, alpha=.75 * np.ones(N), cmap=cmaps[value])
    # Update the color visual
    visual.data('color', color)

# We add a button to regenerate the marker positions
@gui.control("button", "new positions")
def on_change(value):
    pos = nr.randn(N, 3)
    visual.data('pos', pos)

# We run the main event loop, which will display the canvas until Escape is pressed or the
# window is closed.
# We start a screencast. This will add playback buttons at the bottom right corner.
# NOTE: the video DOESN'T START until you press the play button.
# run(video="screencast.mp4")
run()

"""
# Quickstart tutorial with Python

"""

# Imports.
import time
import numpy as np
import numpy.random as nr

# Import the library.
from datoviz import canvas, run, colormap, enable_ipython

# Create a new canvas. The entry point of the Python API is the `app()` function with returns a
# singleton App instance. This object allows to access the GPU(s) available on the system.
# By default, the "best" (most capable) GPU is selected. The GPU allows to create canvases and
# GPU objects. Every canvas is associated to a given GPU for image presentation.
c = canvas(show_fps=True)

# Create a scene, which provides plotting capabilities and allows to organize the canvas into a
# grid of subplots. By default there is only a single panel spanning the whole canvas.
s = c.scene()

# We create a panel with 2D axes.
panel = s.panel(controller='axes')

# We create a new "marker" visual.
visual = panel.visual('marker')

# We prepare the visual properties. Here, we set the marker positions, colors (RGBA bytes),
# and size (in pixels).
N = 100_000
pos = nr.randn(N, 3)
ms = nr.uniform(low=2, high=35, size=N)
color_values = nr.rand(N)

# We use a built-in colormap
color = colormap(color_values, vmin=0, vmax=1, alpha=.75 * np.ones(N), cmap='viridis')

# We set the visual props.
visual.data('pos', pos)
visual.data('color', color)
visual.data('ms', ms)

# We define an event callback to implement mouse picking
@c.connect
def on_mouse_click(x, y, button, modifiers=()):
    # x, y are in pixel coordinates
    # First, we find the picked panel
    p = s.panel_at(x, y)
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

# We run the main rendering loop, which will display the canvas until Escape is pressed or the
# window is closed.
run()

"""
# Quickstart tutorial with Python

"""

# Imports.
import time
import numpy as np
import numpy.random as nr

# Import the library.
from datoviz import canvas, run, colormap

# Create a new canvas.
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
    print(f"Pick at ({xd:.4f}, {yd:.4f}), {'+'.join(modifiers)} {button} click")

# We create a new GUI
gui = c.gui("Test GUI")

# We add a control, a slider controlling a float
sf = gui.control("slider_float", "marker size", vmin=.5, vmax=2)

# We write the Python callback function for when the slider's value changes.
@sf.connect
def on_change(value):
    # Every time the slider value changes, we update the visual's marker size
    visual.data('ms', ms * value)
    # NOTE: an upcoming version will support partial updates

# We add another control, a slider controlling an int between 1 and 4, to change the colormap.
# NOTE: an upcoming version will provide a dropdown menu control.
si = gui.control("slider_int", "colormap", vmin=0, vmax=3)

# Predefined list of colormaps.
cmaps = ['viridis', 'cividis', 'autumn', 'winter']

@si.connect
def on_change(value):
    # We recompute the colors
    color = colormap(color_values, vmin=0, vmax=1, alpha=.75 * np.ones(N), cmap=cmaps[value])
    # We update the color visual
    visual.data('color', color)

# We add a button to regenerate the marker positions
b = gui.control("button", "new positions")

@b.connect
def on_change(value):
    pos = nr.randn(N, 3)
    visual.data('pos', pos)

# We run the main rendering loop, which will display the canvas until Escape is pressed or the
# window is closed.
# NOTE: there are several event loops. By default, the native datoviz event loop is used. It
# is the fastest, but it doesn't allow for interactive use in IPython, and it doesn't support
# asynchronous callbacks.
# In IPython or Jupyter, use the `ipython` event loop (experimental). Otherwise, you can use
# the `asyncio` event loop (experimental).
run(event_loop='asyncio')

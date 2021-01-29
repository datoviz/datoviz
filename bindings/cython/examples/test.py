# Simple test with Python

import numpy.random as nr

# Import the library.
from visky import canvas, run

# Create a new canvas and scene. There's only 1 subplot (panel) by default.
c = canvas(show_fps=True)

# Get a panel (by default, the one spanning the entire canvas)
# We specify the type of controller we want. Here, we want 2D axes.
panel = c.panel(controller='axes')

# We create a new "marker" visual.
visual = panel.visual('marker')

# We set the visual data via "visual properties" (or props). Each visual comes with a set of
# predefined props. Here, we set the marker positions, colors (RGBA bytes), and size (in pixels)
N = 10_000
visual.data('pos', nr.randn(N, 3))
visual.data('color', nr.randint(low=50, high=255, size=(N, 4)))
visual.data('ms', nr.uniform(low=2, high=40, size=N))

# We run the main event loop, which will display the canvas until Escape is pressed or the window
# is closed.
run(screenshot="a.png")

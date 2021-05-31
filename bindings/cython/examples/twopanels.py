"""
# Two panels

This example shows how to display two different panels (2D and 3D) with different controllers side to side.

"""

# Imports.
import numpy as np
import numpy.random as nr
from datoviz import canvas, run, colormap

# Data.
N = 50_000
pos = nr.randn(N, 3)
ms = nr.uniform(low=2, high=40, size=N)
color = colormap(nr.rand(N), vmin=0, vmax=1)

# We create a canvas.
c = canvas(show_fps=True)

# We create a scene with one row and two columns.
s = c.scene(1, 2)

# We add the two panels with different controllers.
panel0 = s.panel(0, 0, controller='axes')
panel1 = s.panel(0, 1, controller='arcball')

# We create a visual in each panel.
visual = panel0.visual('point')
visual.data('pos', pos)
visual.data('color', color)
visual.data('ms', np.array([10]))

visual1 = panel1.visual('point', depth_test=True)
visual1.data('pos', pos)
visual1.data('color', color)

# Start the event loop.
run()

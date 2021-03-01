"""
# Custom colormap

This example shows how to create and use a custom colormap.

"""

import numpy as np
import numpy.random as nr

from datoviz import canvas, run, colormap

c = canvas(show_fps=True)

panel = c.panel(controller='panzoom')
visual = panel.visual('path')

# Create a horizontal thick line.
n = 256
x = np.linspace(-1, 1, n)
y = np.zeros(n)
z = np.zeros(n)
pos = np.c_[x, y, z]

# Create a custom color map, ranging from red to green.
cmap = np.c_[np.arange(256), np.arange(256)[::-1], np.zeros(256), 255 * np.ones(256)]
cmap = cmap.astype(np.uint8)

# Register the custom colormap.
c.colormap('mycmap', cmap)

# Use the custom colormap in the visual.
color = colormap(np.linspace(0, 1, n), cmap='mycmap')

visual.data('pos', pos)
visual.data('color', color)
visual.data('linewidth', np.array([50]))
visual.data('cap_type', np.array([0]))

run(screenshot='colormap.png')

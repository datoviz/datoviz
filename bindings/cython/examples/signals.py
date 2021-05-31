"""
# Digital signals

This example shows how to display a large number of low-quality paths with dynamic updates, similar
to a real-time digital acquisition system.

"""

# Imports.
import numpy as np
import numpy.random as nr
from datoviz import canvas, run, colormap

# Create the canvas, panel, and visual.
c = canvas(show_fps=True)
panel = c.scene().panel(controller='panzoom')
visual = panel.visual('line_strip', transform=None)

# Parameters.
n_signals = 100
n_points = 2_000
n_vert = n_signals * n_points

# Create the signals data.
t = np.linspace(-1, 1, n_points)
x = np.tile(t, (n_signals,))
assert x.ndim == 1

coef = .5 / n_signals
y = coef * nr.randn(n_signals, n_points)
offsets = np.tile(np.linspace(-1, +1, n_signals)[:, np.newaxis], (1, n_points))
y += offsets
pos = np.c_[x, y.ravel(), np.zeros(n_vert)]

# Signal colors.
color = np.repeat(colormap(np.linspace(0, 1, n_signals), cmap='glasbey'), n_points, axis=0)

# Length of each signal (batch visual).
length = np.repeat(np.array([n_points]), n_signals)

assert pos.shape == (n_vert, 3)
assert color.shape == (n_vert, 4)
assert length.shape == (n_signals,)

# Set the visual data.
visual.data('pos', pos)
visual.data('color', color)
visual.data('length', length)

# Animation function.
k = 10

@c.connect
def on_frame(i):
    # This function runs at every frame.
    i = i % (n_points // k)
    yk = coef * nr.randn(n_signals, k)
    offsets = np.tile(np.linspace(-1, +1, n_signals)[:, np.newaxis], (1, k))
    yk += offsets
    y[:, i * k:(i + 1) * k] = yk
    pos[:, 1] = y.ravel()
    visual.data('pos', pos)

run()

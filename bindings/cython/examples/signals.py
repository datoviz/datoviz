"""
# Digital signals

"""

import numpy as np
import numpy.random as nr

from datoviz import canvas, run, colormap

c = canvas(show_fps=True)

panel = c.scene().panel(controller='panzoom')
visual = panel.visual('line_strip', transform=None)

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

color = np.repeat(colormap(np.linspace(0, 1, n_signals), cmap='glasbey'), n_points, axis=0)
length = np.repeat(np.array([n_points]), n_signals)

assert pos.shape == (n_vert, 3)
assert color.shape == (n_vert, 4)
assert length.shape == (n_signals,)

visual.data('pos', pos)
visual.data('color', color)
visual.data('length', length)

# Animation function.
k = 10

@c.connect
def on_frame(i):
    i = i % (n_points // k)
    yk = coef * nr.randn(n_signals, k)
    offsets = np.tile(np.linspace(-1, +1, n_signals)[:, np.newaxis], (1, k))
    yk += offsets
    y[:, i * k:(i + 1) * k] = yk
    pos[:, 1] = y.ravel()
    visual.data('pos', pos)

run()

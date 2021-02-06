import numpy as np
import numpy.random as nr
from datoviz import canvas, run, colormap

c = canvas(show_fps=False)
panel = c.panel(controller='arcball')
visual = panel.visual('mesh')

n = 100
t = np.linspace(-1, 1)
x, y = np.meshgrid(t, t)
z = .5 * np.sin(10 * x) * np.cos(10 * y)
z *= np.exp(-1 * (x ** 2 + y ** 2))
assert x.shape == y.shape == z.shape
assert x.ndim == 2

visual.surface(x, y, z)

run()

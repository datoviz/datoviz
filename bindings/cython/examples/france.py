import numpy as np
import numpy.random as nr
from datoviz import canvas, run, colormap

pos = np.fromfile("data/misc/departements.polypoints.bin", dtype=np.float64)
pos = pos.reshape((-1, 2))
pos = np.c_[pos, np.zeros(pos.shape[0])]

length = np.fromfile("data/misc/departements.polylengths.bin", dtype=np.uint32)
N = len(length)
color = colormap(nr.rand(N), vmin=0, vmax=1, cmap='viridis')

c = canvas(show_fps=False)
panel = c.panel(controller='axes')
visual = panel.visual('polygon')

visual.data('pos', pos)
visual.data('length', length)
visual.data('color', color)

run()

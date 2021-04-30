from pathlib import Path
import numpy as np
import numpy.random as nr
from datoviz import app, next_frame, canvas, run, colormap

c = canvas(show_fps=True)
panel = c.scene().panel(controller='axes')
visual = panel.visual('marker')

N = 10_000
pos = nr.randn(N, 3)
ms = nr.uniform(low=2, high=35, size=N)
color_values = nr.rand(N)
color = colormap(color_values, vmin=0, vmax=1, alpha=.75 * np.ones(N), cmap='viridis')

visual.data('pos', pos)
visual.data('color', color)
visual.data('ms', ms)

while next_frame():
    pass
print("finished")

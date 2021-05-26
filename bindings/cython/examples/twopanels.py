"""
# Test with two panels and different controllers

"""

import numpy as np
import numpy.random as nr

from datoviz import canvas, run, colormap


N = 50_000
pos = nr.randn(N, 3)
ms = nr.uniform(low=2, high=40, size=N)
color = colormap(nr.rand(N), vmin=0, vmax=1)


c = canvas(show_fps=True)
s = c.scene(1, 2)

panel0 = s.panel(0, 0, controller='axes')
panel1 = s.panel(0, 1, controller='arcball')

visual = panel0.visual('marker')
visual.data('pos', pos)
visual.data('color', color)
visual.data('ms', ms)

visual1 = panel1.visual('marker', depth_test=True)
visual1.data('pos', pos)
visual1.data('color', color)
visual1.data('ms', ms)

run()

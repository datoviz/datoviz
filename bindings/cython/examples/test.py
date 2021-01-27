import time
import numpy as np
import numpy.random as nr
from visky import canvas, run

c = canvas()
panel = c.panel()
visual = panel.visual('marker')

N = 10_000
visual.data('pos', nr.randn(N, 3))
visual.data('color', nr.randint(low=50, high=255, size=(N, 4)))
visual.data('ms', nr.uniform(low=5, high=50, size=N))

run()

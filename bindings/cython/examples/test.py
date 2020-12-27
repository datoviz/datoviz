import time
import numpy as np
from visky import canvas, run

c = canvas()
panel = c.panel()
visual = panel.visual('marker')

N = 100
visual.data('pos', (.25 * np.random.randn(N, 3)).astype(np.float32))
visual.data('color', np.random.randint(
    low=100, high=255, size=(N, 4)).astype(np.uint8))
visual.data('ms', (50 * np.random.rand(1)).astype(np.float32))

run()
# app.run()
# app.destroy()

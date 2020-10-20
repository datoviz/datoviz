import numpy as np
from visky import pyvisky

app = pyvisky.App()
c = app.canvas()
s = c.scene()
p = s.panel()
p.set_controller()
v = p.visual('marker')
v.data('pos', .25 * np.random.randn(100, 3).astype(np.float32))
app.run()
del app

import numpy as np
from visky import pyvisky

app = pyvisky.App()
c = app.canvas()
s = c.scene()

p = s.panel()
p.set_controller('axes')

v = s.visual('marker')
v.add_to_panel(p)

n = 10000
v.data('pos', .25 * np.random.randn(n, 3).astype(np.float32))
v.data('color', np.random.randint(
    size=(n, 4), low=100, high=255).astype(np.uint8))
v.data('size', np.random.uniform(size=n, low=10, high=50).astype(np.float32))

app.run()
del app

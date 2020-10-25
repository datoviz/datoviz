import numpy as np
from visky import canvas, run, enable_ipython

enable_ipython()

# Test data
n = 10000
pos = .25 * np.random.randn(n, 3).astype(np.float32)
color = np.random.randint(size=(n, 4), low=100, high=255).astype(np.uint8)
size = np.random.uniform(size=n, low=10, high=50).astype(np.float32)

# Make the canvas and create the marker visual
c = canvas(cols=2)
c[0, 0].axes().markers(pos=pos, color=color, size=size)
c[0, 1].axes().imshow(np.random.uniform(
    size=(128, 32, 4), low=0, high=255).astype(np.uint8))

# Start the event loop
run()

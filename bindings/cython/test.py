import numpy as np
from visky.pyvisky import canvas, run

# Test data
n = 10000
pos = .25 * np.random.randn(n, 3).astype(np.float32)
color = np.random.randint(size=(n, 4), low=100, high=255).astype(np.uint8)
size = np.random.uniform(size=n, low=10, high=50).astype(np.float32)

# Make the canvas and create the marker visual
# canvas()[0, 0].axes().markers(pos=pos, color=color, size=size)
img = np.random.uniform(size=(512, 512), low=0, high=255).astype(np.uint8)
canvas()[0, 0].axes().imshow(img)

# Start the event loop
run()

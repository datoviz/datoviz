import numpy as np
import numpy.random as nr
from datoviz import canvas, run, colormap

c = canvas(show_fps=True)
panel = c.panel(controller='panzoom')
visual = panel.visual('image')

# Top left, top right, bottom right, bottom left
visual.data('pos', np.array([[-1, +1, 0]]), idx=0)
visual.data('pos', np.array([[+1, +1, 0]]), idx=1)
visual.data('pos', np.array([[+1, -1, 0]]), idx=2)
visual.data('pos', np.array([[-1, -1, 0]]), idx=3)

visual.data('texcoords', np.atleast_2d([0, 0]), idx=0)
visual.data('texcoords', np.atleast_2d([1, 0]), idx=1)
visual.data('texcoords', np.atleast_2d([1, 1]), idx=2)
visual.data('texcoords', np.atleast_2d([0, 1]), idx=3)

n = 256

# First texture.
img = nr.randint(low=0, high=255, size=(n, n, 4)).astype(np.uint8)
img[:, :, 3] = 255
visual.image(img, filtering='nearest', idx=0)

# Second texture.
t = np.linspace(-1, +1, n)
x, y = np.meshgrid(t, t)
z = np.exp(-x * x - y * y)
z = (z * 255).astype(np.uint8)
img[:, :, :3] = z[..., np.newaxis]
img[:, :, 3] = 255
visual.image(img, filtering='nearest', idx=1)

visual.data('texcoefs', np.array([1, .5, 0, 0]).astype(np.float32))

# Control the blending via a GUI.
gui = c.gui("GUI")

@gui.control("slider_float", "blending", vmin=0, vmax=1)
def on_change(value):
    visual.data('texcoefs', np.array([1 - value, value, 0, 0]))

run()

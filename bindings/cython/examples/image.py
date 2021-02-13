"""
# Image

"""

from pathlib import Path
import numpy as np
import numpy.random as nr
import imageio


from datoviz import canvas, run, colormap

ROOT = Path(__file__).parent.parent.parent.parent

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

# First texture.
img = imageio.imread(ROOT / 'data/textures/earth.jpg')
img = np.dstack((img, 255 * np.ones(img.shape[:2])))
img = np.transpose(img, (1, 0, 2))
img = img.astype(np.uint8)
visual.image(img, filtering='nearest', idx=0)

# Second texture.
n = 256
t = np.linspace(-1, +1, n)
x, y = np.meshgrid(t, t)
z = np.exp(-2 * (x * x + y * y))
z = (z * 255).astype(np.uint8)
img = np.dstack((z, z, z, 255 * np.ones_like(z))).astype(np.uint8)
visual.image(img, filtering='nearest', idx=1)

visual.data('texcoefs', np.array([1, .5, 0, 0]).astype(np.float32))

# Control the blending via a GUI.
gui = c.gui("GUI")


@gui.control("slider_float", "blending", vmin=0, vmax=1)
def on_change(value):
    visual.data('texcoefs', np.array([1 - value, value, 0, 0]))


run()

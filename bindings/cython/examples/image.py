"""
# Image

This example shows how to display two superimposed images of different sizes
(but automatically rescaled), with simple blending done on the GPU, with a slider
controlling the blending parameter.

"""

from pathlib import Path
import numpy as np
import numpy.random as nr
import imageio

from datoviz import app, canvas, run, colormap

ROOT = Path(__file__).resolve().parent.parent.parent.parent

def load_image(path):
    """This function loads an image with imageio, and uploads it to a new GPU texture."""
    img = imageio.imread(path)
    img = np.dstack((img, 255 * np.ones(img.shape[:2])))
    img = img.astype(np.uint8)
    tex = app().gpu().context().texture(img.shape[0], img.shape[1])
    tex.upload(img)
    return tex

c = canvas(show_fps=True)
panel = c.scene().panel(controller='panzoom')
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
tex0 = load_image(ROOT / 'data/textures/earth.jpg')
visual.texture(tex0, idx=0)  # set the first texture slot of the image visual

# Second texture.
tex1 = load_image(ROOT / 'data/textures/landscape.jpg')
visual.texture(tex1, idx=1)  # set the second texture slot of the image visual

visual.data('texcoefs', np.array([1, 0, 0, 0]).astype(np.float32))

# Control the blending via a GUI.
gui = c.gui("GUI")

slider = gui.control("slider_float", "GPU blending", value=0, vmin=0, vmax=1)

@slider.connect
def on_change(value):
    # Convex combination of the two images.
    visual.data('texcoefs', np.array([1 - value, value, 0, 0]))

run()

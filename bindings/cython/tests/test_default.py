
# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from pathlib import Path
import time

import numpy as np
from numpy.testing import assert_array_equal as ae
import numpy.random as nr
from pytest import fixture
import imageio

from datoviz import App, app, canvas, colormap

ROOT_PATH = Path(__file__).resolve().parent.parent.parent.parent
print(ROOT_PATH)


# -------------------------------------------------------------------------------------------------
# Test utils
# -------------------------------------------------------------------------------------------------

def clear_loggers():
    """Remove handlers from all loggers"""
    import logging
    loggers = [logging.getLogger()] + list(logging.Logger.manager.loggerDict.values())
    for logger in loggers:
        handlers = getattr(logger, 'handlers', [])
        for handler in handlers:
            logger.removeHandler(handler)


def teardown():
    # HACK: fixes pytest bug
    # see https://github.com/pytest-dev/pytest/issues/5502#issuecomment-647157873
    clear_loggers()


# -------------------------------------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------------------------------------

# HACK: calling the gpu.context() automatically creates a context with NO support for surface if
# there isn't already a context. A crash occurs when trying to create a canvas AFTER a context
# with NO support for surface has been created. At the moment the context can only be created
# once.

def test_gpu():
    a = app()
    print(a)

    g = a.gpu()
    assert g.name
    print(g)
    assert str(g).startswith("<GPU")



def test_canvas():
    c = canvas()
    app().run(10)



def test_colormap():
    n = 1000
    values = np.linspace(-1, 1, n)
    alpha = np.linspace(+1, +.5, n)
    colors = colormap(values, vmin=-1, vmax=+1, cmap='viridis', alpha=alpha)
    assert colors.dtype == np.uint8
    assert colors.shape == (n, 4)
    ae(colors[0], (68, 1, 84, 255))
    ae(colors[-1], (253, 231, 36, 127))



def test_texture():
    ctx = app().gpu().context()
    print(ctx)
    assert str(ctx).startswith("<Context")

    # Create texture.
    h, w = 16, 32
    tex = ctx.texture(h, w)
    assert tex.item_size == 1
    assert tex.shape == (h, w, 4)
    assert tex.size == h * w * 4
    print(tex)
    assert str(tex) == f"<Texture 2D {h}x{w}x4 (uint8)>"

    # Upload texture data.
    arr = nr.randint(low=0, high=255, size=(h, w, 4)).astype(np.uint8)
    tex.upload(arr)

    # Download the data.
    arr2 = tex.download()

    # Check.
    assert arr2.dtype == arr.dtype
    assert arr2.shape == arr.shape
    ae(arr2, arr)



def test_gui_demo():
    c = canvas()
    c.gui_demo()
    app().run(10)
    c.close()



def test_gui_visual_marker():
    c = canvas()
    s = c.scene()
    p = s.panel()
    v = p.visual('marker')
    n = 10000
    nr.seed(0)
    v.data('pos', np.c_[nr.normal(size=(n, 2)), np.zeros(n)])
    v.data('ms', nr.uniform(size=n, low=5, high=30))
    v.data('color', nr.randint(size=(n, 4), low=100, high=255))
    app().run(10)
    c.close()



def test_gui_visual_image():
    a = app()
    g = a.gpu()
    c = g.canvas()
    ctx = g.context()
    s = c.scene()
    p = s.panel(controller='panzoom')
    v = p.visual('image')

    # Top left, top right, bottom right, bottom left
    v.data('pos', np.array([[-1, +1, 0]]), idx=0)
    v.data('pos', np.array([[+1, +1, 0]]), idx=1)
    v.data('pos', np.array([[+1, -1, 0]]), idx=2)
    v.data('pos', np.array([[-1, -1, 0]]), idx=3)

    v.data('texcoords', np.atleast_2d([0, 0]), idx=0)
    v.data('texcoords', np.atleast_2d([1, 0]), idx=1)
    v.data('texcoords', np.atleast_2d([1, 1]), idx=2)
    v.data('texcoords', np.atleast_2d([0, 1]), idx=3)

    # First texture.
    img = imageio.imread(ROOT_PATH / 'data/textures/earth.jpg')
    img = np.dstack((img, 255 * np.ones(img.shape[:2])))
    img = img.astype(np.uint8)
    tex = ctx.texture(img.shape[0], img.shape[1])
    tex.upload(img)
    v.texture(tex)

    app().run(10)
    c.close()



def test_subplots():
    c = canvas(show_fps=True)
    s = c.scene(1, 2)
    p0, p1 = s.panel(col=0), s.panel(col=1, controller='arcball')
    n = 10000
    for p in (p0, p1):
        nr.seed(0)
        v = p.visual('marker', depth_test=p == p1)
        v.data('pos', nr.normal(size=(n, 3)))
        v.data('ms', nr.uniform(size=n, low=5, high=30))
        v.data('color', colormap(nr.rand(n), alpha=.75))
    app().run(10)

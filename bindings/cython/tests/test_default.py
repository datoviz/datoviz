
# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import logging
import os
from pathlib import Path
import shutil
import time

import numpy as np
from numpy.testing import assert_array_equal as ae
import numpy.random as nr
from pytest import fixture
import imageio

from datoviz import app, canvas, colormap, run, add_default_handler

logger = logging.getLogger('datoviz')
# add_default_handler('DEBUG')


ROOT_PATH = Path(__file__).resolve().parent.parent.parent.parent
CYTHON_PATH = Path(__file__).resolve().parent.parent
IMAGES_PATH = CYTHON_PATH / 'images'


# -------------------------------------------------------------------------------------------------
# Test utils
# -------------------------------------------------------------------------------------------------

def clear_loggers():
    """Remove handlers from all loggers"""
    loggers = [logging.getLogger()] + list(logging.Logger.manager.loggerDict.values())
    for logger in loggers:
        handlers = getattr(logger, 'handlers', [])
        for handler in handlers:
            logger.removeHandler(handler)


def teardown():
    # HACK: fixes pytest bug
    # see https://github.com/pytest-dev/pytest/issues/5502#issuecomment-647157873
    logger.debug("Teardown.")
    clear_loggers()


def check_screenshot(filename):
    assert filename.exists
    filename_ref = filename.with_suffix('').with_suffix('').with_suffix('.png')
    if not filename_ref.exists():
        logger.debug(f"Reference image {filename_ref} didn't exist, skipping image check.")
        shutil.copy(filename, filename_ref)
        return True
    img_new = imageio.imread(filename)
    img_ref = imageio.imread(filename_ref)
    if img_new.shape != img_ref.shape:
        logger.debug(f"Image size is different: {img_new.shape} != {img_ref.shape}")
        return False
    return np.all(img_new == img_ref)


def check_canvas(ca, test_name):
    if not IMAGES_PATH.exists():
        IMAGES_PATH.mkdir(exist_ok=True, parents=True)
    screenshot = IMAGES_PATH / f'{test_name}.new.png'

    # Interactive mode if debug.
    debug = os.environ.get('DVZ_DEBUG', None)
    if debug:
        app().run()
        ca.close()
        return

    # Run and save the screenshot.
    app().run(10, screenshot=str(screenshot))

    # Close the canvas.
    ca.close()

    # Check the screenshot.
    res = check_screenshot(screenshot)
    assert res, f"Screenshot check failed for {test_name}"

    # Delete the new screenshot if it matched the reference image.
    if res:
        logger.debug(f"Screenshot check succeedeed for {test_name}")
        os.remove(screenshot)


@fixture
def c(request):
    # Create the canvas.
    ca = canvas()
    yield ca
    # Check screenshot.
    check_canvas(ca, request.node.name)


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



def test_canvas(c):
    assert c



def test_colormap():
    n = 1000
    values = np.linspace(-1, 1, n)
    alpha = np.linspace(+1, +.5, n)
    colors = colormap(values, vmin=-1, vmax=+1, cmap='viridis', alpha=alpha)
    assert colors.dtype == np.uint8
    assert colors.shape == (n, 4)
    ae(colors[0], (68, 1, 84, 255))
    ae(colors[-1], (253, 231, 36, 127))



def test_colormap_custom():
    # Create a custom color map, ranging from red to green.
    cmap = np.c_[np.arange(256), np.arange(256)[::-1], np.zeros(256), 255 * np.ones(256)]
    cmap = cmap.astype(np.uint8)

    # Register the custom colormap.
    app().gpu().context().colormap('mycmap', cmap)

    # Test the custom colormap.
    n = 256
    colors = colormap(np.linspace(0, 1, n), cmap='mycmap')
    ae(colors[:, 0], np.arange(256))
    ae(colors[::-1, 1], np.arange(256))
    assert np.all(colors[:, 2] == 0)



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



# -------------------------------------------------------------------------------------------------
# Tests with canvas
# -------------------------------------------------------------------------------------------------

def test_gui_demo(c):
    c.gui_demo()



def test_gui_custom(c):
    gui = c.gui("Test GUI")

    gui.control("slider_float", "slider float", vmin=0, vmax=10)
    gui.control("slider_int", "slider int", vmin=0, vmax=3)
    button = gui.control("button", "click me")

    _clicked = []

    @button.connect
    def on_click(value):
        print("clicked!")
        _clicked.append(0)

    @c.connect
    def on_frame(idx):
        if idx == 3:
            x, y = button.pos
            w, h = button.size
            x += w / 2.
            y += h / 2.
            c.click(x, y)
            # HACK: manual click doesn't work with Dear ImGui, need to simulate GUI actions
            # manually
            button.press()
        if idx == 6:
            assert _clicked and _clicked[0] == 0



def test_visual_marker(c):
    s = c.scene()
    p = s.panel()
    v = p.visual('marker')
    n = 10000
    nr.seed(0)
    v.data('pos', np.c_[nr.normal(size=(n, 2)), np.zeros(n)])
    v.data('ms', nr.uniform(size=n, low=5, high=30))
    v.data('color', nr.randint(size=(n, 4), low=100, high=255))



def test_visual_image(c):
    ctx = c.gpu().context()
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



def test_subplots(c):
    s = c.scene(1, 2)
    p0, p1 = s.panel(col=0), s.panel(col=1, controller='arcball')
    n = 10000
    for p in (p0, p1):
        nr.seed(0)
        v = p.visual('marker', depth_test=p == p1)
        v.data('pos', nr.normal(size=(n, 3)))
        v.data('ms', nr.uniform(size=n, low=5, high=30))
        v.data('color', colormap(nr.rand(n), alpha=.75))



def test_event_loop():
    c = canvas(show_fps=True)
    for event_loop in ('native', 'asyncio'):
        run(10, event_loop=event_loop)
    c.close()

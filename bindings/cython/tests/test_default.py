
# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import time

import numpy as np
from numpy.testing import assert_array_equal as ae
import numpy.random as nr
from pytest import fixture

from datoviz import App, app, canvas


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
    # c.close()
    # app().run(1)
    # del c
    # c.destroy()



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

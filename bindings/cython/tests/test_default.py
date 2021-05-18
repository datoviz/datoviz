
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

def test_gpu():
    a = app()

    g = a.gpu()
    assert g.name
    print(g)
    assert str(g).startswith("<GPU")

    ctx = g.context()
    print(ctx)
    assert str(ctx).startswith("<Context")


def test_canvas():
    c = canvas()
    app().run(10)
    c.destroy()


def test_texture():
    context = app().gpu().context()

    # Create texture.
    h, w = 16, 32
    tex = context.texture(h, w)
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

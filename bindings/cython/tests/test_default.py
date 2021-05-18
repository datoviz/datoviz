
# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import time

import numpy as np
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

    arr = nr.randint(low=0, high=255, size=(16, 32, 4)).astype(np.uint8)
    tex = context.image(arr)

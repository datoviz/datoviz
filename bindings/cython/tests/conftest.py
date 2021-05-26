
# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import logging
import os
from pathlib import Path

from pytest import fixture

from datoviz import canvas
from .utils import check_canvas

logger = logging.getLogger('datoviz')

CUR_DIR = Path(__file__).parent



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


def pytest_sessionstart(session):
    path = CUR_DIR / '../imgui.ini'
    if path.exists():
        os.remove(path)


def pytest_sessionfinish(session, exitstatus):
    # HACK: fixes pytest bug
    # see https://github.com/pytest-dev/pytest/issues/5502#issuecomment-647157873
    logger.debug("Session-level pytest teardown.")
    clear_loggers()


@fixture
def c(request):
    # Create the canvas.
    ca = canvas()
    yield ca
    # Check screenshot.
    check_canvas(ca, request.node.name)

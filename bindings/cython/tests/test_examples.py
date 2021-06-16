
# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import logging
import os
from pathlib import Path
import shutil
import time
from runpy import run_path

import numpy as np
from numpy.testing import assert_array_equal as ae
import numpy.random as nr
from pytest import fixture
import imageio

import datoviz
from datoviz import canvas as canvas_real
from .utils import check_canvas, ROOT_PATH, SCREENSHOTS_PATH, CYTHON_PATH

CUR_DIR = Path(__file__).parent

logger = logging.getLogger('datoviz')


# -------------------------------------------------------------------------------------------------
# Test utils
# -------------------------------------------------------------------------------------------------

def mock_run(*args, **kwargs):
    logger.debug("mock run")


def canvas_nofps(*args, **kwargs):
    kwargs.pop('show_fps', None)
    if not os.environ.get('DVZ_DEBUG', None):
        kwargs['offscreen'] = True
    return canvas_real(*args, **kwargs)


def check_example(name):
    glob = globals()

    # HACK: monkey patch of the run function to control the execution.
    glob['datoviz'].run = mock_run
    glob['datoviz'].canvas = canvas_nofps

    nr.seed(0)
    ret = run_path(CUR_DIR.parent / f'examples/{name}.py', init_globals=glob)
    c = ret.get('c', None)
    assert c
    check_canvas(c, f'py_{name}', output_dir=SCREENSHOTS_PATH)


# -------------------------------------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------------------------------------

def pytest_generate_tests(metafunc):
    if 'example_name' in metafunc.fixturenames:
        example_names = sorted(CYTHON_PATH.glob('examples/*.py'))
        example_names = [_.stem for _ in example_names]
        metafunc.parametrize("example_name", example_names)


def test_example(example_name):
    check_example(example_name)

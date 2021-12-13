# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import logging
import os
from pathlib import Path

import numpy as np
from numpy.testing import assert_array_equal as ae
import numpy.random as nr

from datoviz import Renderer
from .utils import ROOT_PATH


# -------------------------------------------------------------------------------------------------
# Logger
# -------------------------------------------------------------------------------------------------

logger = logging.getLogger('datoviz')


# -------------------------------------------------------------------------------------------------
# CONSTANTS
# -------------------------------------------------------------------------------------------------

WIDTH = 800
HEIGHT = 600


# -------------------------------------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------------------------------------

def test_renderer_1():
    r = Renderer()
    with r.requests() as requests:
        board = r._create_board(WIDTH, HEIGHT)
        graphics = r._create_graphics(board, 4)
        dat = r._create_dat(2, 48)
        r._set_vertex(graphics, dat)
        arr = np.array([
            (-1., -1., 0., 255,  0,  0, 255),
            ( 1., -1., 0.,  0, 255,  0, 255),
            ( 0.,  1., 0.,  0,  0, 255, 255)],
        dtype=[
            ('x', '<f4'), ('y', '<f4'), ('z', '<f4'),
            ('r', 'i1'), ('g', 'i1'), ('b', 'i1'), ('a', 'i1')])
        r._upload_dat(dat, 0, arr)

        r._set_begin(board)
        r._set_viewport(board, 0, 0, WIDTH, HEIGHT)
        r._set_draw(board, graphics, 0, 3)
        r._set_end(board)
        r._update_board(board)
    print(requests)

    # r.save_image(board, str(ROOT_PATH / "../build/artifacts/pyrenderer.png"))

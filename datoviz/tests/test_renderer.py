# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import logging
import os
from pathlib import Path

import numpy as np
from numpy.testing import assert_array_equal as ae
import numpy.random as nr

from datoviz import Requester, Renderer
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
    rq = Requester()
    with rq.requests():
        board = rq.create_board(WIDTH, HEIGHT)
        # HACK: create MVP and viewport dat
        graphics = rq.create_graphics(board, 4, flags=3)
        dat = rq.create_dat(2, 48)
        rq.set_vertex(graphics, dat)
        arr = np.array([
            (-1., -1., 0., 255,  0,  0, 255),
            (1., -1., 0.,  0, 255,  0, 255),
            (0.,  1., 0.,  0,  0, 255, 255)],
            dtype=[
            ('x', '<f4'), ('y', '<f4'), ('z', '<f4'),
            ('r', 'i1'), ('g', 'i1'), ('b', 'i1'), ('a', 'i1')])
        rq.upload_dat(dat, 0, arr)

        rq.set_begin(board)
        rq.set_viewport(board, 0, 0, WIDTH, HEIGHT)
        rq.set_draw(board, graphics, 0, 3)
        rq.set_end(board)
        rq.update_board(board)

    rd = Renderer()
    rq.submit(rd)

    rd.save_image(board, str(ROOT_PATH / "../build/artifacts/pyrenderer.png"))

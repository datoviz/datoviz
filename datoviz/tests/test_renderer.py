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

        rq.record_begin(board)
        rq.record_viewport(board, 0, 0, WIDTH, HEIGHT)
        rq.record_draw(board, graphics, 0, 3)
        rq.record_end(board)
        rq.update_board(board)

    rd = Renderer()
    rq.submit(rd)

    rd.save_image(board, str(
        ROOT_PATH / "../build/artifacts/pyrenderer_1.png"))


def test_renderer_image():
    rq = Requester()
    with rq.requests():
        board = rq.create_board(WIDTH, HEIGHT)
        # HACK: create MVP and viewport dat
        graphics = rq.create_graphics(board, 12, flags=3)

        # Vertex buffer
        dat = rq.create_dat(2, 6 * 20)
        rq.set_vertex(graphics, dat)

        # Params slot #2
        params = rq.create_dat(5, 16)
        rq.upload_dat(params, 0, np.array([1, 0, 0, 0], dtype=np.float32))
        rq.bind_dat(graphics, 2, params)

        # Tex slot #3
        w = 64
        h = 48
        tex = rq.create_tex(2, 37, w, h, 1)
        img = np.random.uniform(
            low=92, high=230, size=(w, h, 4)).astype(np.uint8)
        img[..., 3] = 255
        rq.upload_tex(tex, img, w=w, h=h)

        # Sampler
        sampler = rq.create_sampler(0, 0)
        for i in range(3, 3+4):
            rq.bind_tex(graphics, i, tex, sampler)

        # Vertex data.
        arr = np.array([
            ((-1, -1, 0), (0, 1)),
            ((+1, -1, 0), (1, 1)),
            ((+1, +1, 0), (1, 0)),
            ((+1, +1, 0), (1, 0)),
            ((-1, +1, 0), (0, 0)),
            ((-1, -1, 0), (0, 1)),
        ],
            dtype=[
            ('pos', ('<f4', 3)),
            ('uv', ('<f4', 2)),
        ])
        rq.upload_dat(dat, 0, arr)

        # Commands
        rq.record_begin(board)
        rq.record_viewport(board, 0, 0, WIDTH, HEIGHT)
        rq.record_draw(board, graphics, 0, 6)
        rq.record_end(board)
        rq.update_board(board)

    rd = Renderer()
    rq.submit(rd)

    rd.save_image(board, str(
        ROOT_PATH / "../build/artifacts/pyrenderer_image.png"))

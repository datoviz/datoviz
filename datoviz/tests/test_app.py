# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import logging
import os
from pathlib import Path

import numpy as np

from datoviz.app import App
from datoviz.tests.utils import ROOT_PATH


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

def test_app_1():
    app = App()
    canvas = app.canvas(flags=3)
    view = canvas.view()

    n = 50_000_000
    pixel = app.pixel(n)

    pos = np.zeros((n, 3), dtype=np.float32)
    pos[:, :2] = .25 * np.random.randn(n, 2)
    pixel.position(pos)

    color = np.zeros((n, 4), dtype=np.uint8)
    color[:, :3] = np.random.randint(low=100, high=255, size=(n, 3))
    color[:, 3] = 10
    pixel.color(color)

    view.add(pixel)

    canvas.build()
    app.run()


if __name__ == '__main__':
    test_app_1()

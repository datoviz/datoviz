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
# Constants
# -------------------------------------------------------------------------------------------------

WIDTH = 800
HEIGHT = 600


# -------------------------------------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------------------------------------

def test_app_1():
    np.random.seed(0)

    app = App()
    scene = app.scene()
    fig = scene.figure(512, 512, 3)

    n = 1_000
    visual = app.visual(n)

    pos = np.zeros((n, 3), dtype=np.float32)

    pos[:, :2] = -1+2*np.random.rand(n, 2)
    visual.initial(pos)

    pos[:, :2] = -1+2*np.random.rand(n, 2)
    visual.terminal(pos)

    linewidth = 3.0 * np.ones(n, dtype=np.float32)
    visual.linewidth(linewidth)

    color = np.zeros((n, 4), dtype=np.uint8)
    # color[:, :3] = np.random.randint(low=100, high=255, size=(n, 3))
    color[:, 3] = 255
    visual.color(color)

    fig.visual(visual)

    # view.add(visual)

    # canvas.build()
    # app.run()
    scene.run()


if __name__ == '__main__':
    test_app_1()

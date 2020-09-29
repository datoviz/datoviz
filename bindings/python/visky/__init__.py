import numpy as np

from .wrap import viskylib as vl, make_vertices, upload_data
from . import _constants as const
from . import _types as tp
from .api import App, app, canvas, run, get_color


def demo():
    c = canvas()

    n = 1000
    t = np.linspace(-1, 1, n)

    points = np.zeros((n, 3), dtype=np.float32)
    points[:, 0] = t
    points[:, 1] = .5 * np.cos(20 * t)

    colors = get_color('jet', np.linspace(0, 1, n))

    v_plot = c[0, 0].plot(points, colors=colors, lw=10)

    run()

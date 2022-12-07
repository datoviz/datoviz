# cython: c_string_type=unicode, c_string_encoding=ascii

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

# from .renderer cimport Renderer
from . cimport _types as tp
from . cimport app as pt
from . cimport scene as sc
from . cimport fileio
from libc.stdlib cimport free
from cython.view cimport array
import numpy as np
import logging
from contextlib import contextmanager

cimport numpy as np


logger = logging.getLogger('datoviz')


# -------------------------------------------------------------------------------------------------
# Enums
# -------------------------------------------------------------------------------------------------


# -------------------------------------------------------------------------------------------------
# Proto
# -------------------------------------------------------------------------------------------------

def proto():
    cdef pt.DvzApp * _c_app = pt.dvz_app(pt.DVZ_BACKEND_GLFW)
    cdef pt.DvzDevice * _c_device = pt.dvz_device(_c_app)

    cdef sc.DvzScene * _c_scene = sc.dvz_scene()
    cdef sc.DvzFigure * _c_fig = sc.dvz_figure(_c_scene, 800, 600, 1, 1, 0)

    n = 50

    cdef sc.DvzPanel * _c_panel = sc.dvz_panel(_c_fig, 0, 0, tp.DVZ_PANEL_TYPE_NONE, 0)
    cdef sc.DvzVisual * _c_visual = sc.dvz_visual(_c_scene, tp.DVZ_VISUAL_POINT, 0)

    cdef np.ndarray arr
    arr = np.zeros(n,
                   dtype=[('pos', 'f4', 3), ('color', 'u1', 4), ('size', 'f4')])
    t = np.linspace(-1, 1, n, dtype=np.float32)
    arr['pos'] = .75 * np.c_[np.cos(np.pi*t), np.sin(np.pi*t), np.zeros(n)]
    arr['color'][:] = 255
    arr['size'][:] = 10.0
    cdef tp.uint8_t * data = <tp.uint8_t*> & arr.data[0]

    sc.dvz_visual_data(_c_visual, tp.DVZ_PROP_NONE, 0, n, data)
    sc.dvz_panel_visual(_c_panel, _c_visual, 0)

    pt.dvz_device_run(_c_device, _c_scene, 0)

    sc.dvz_figure_destroy(_c_fig)
    sc.dvz_scene_destroy(_c_scene)

    pt.dvz_device_destroy(_c_device)
    pt.dvz_app_destroy(_c_app)

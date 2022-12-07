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
    pt.dvz_device_run(_c_device, _c_scene, 0)
    sc.dvz_figure_destroy(_c_fig)
    sc.dvz_scene_destroy(_c_scene)

    pt.dvz_device_destroy(_c_device)
    pt.dvz_app_destroy(_c_app)

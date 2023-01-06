# cython: c_string_type=unicode, c_string_encoding=ascii
# OBSOLETE, TO REMOVE

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

cdef class Proto:
    cdef pt.DvzApp * _c_app
    cdef pt.DvzDevice * _c_device
    cdef sc.DvzScene * _c_scene
    cdef sc.DvzFigure * _c_fig
    cdef sc.DvzPanel * _c_panel
    cdef sc.DvzVisual * _c_visual

    cdef np.ndarray _arr

    def __init__(self):
        self._c_app = pt.dvz_app(pt.DVZ_BACKEND_GLFW)
        self._c_device = pt.dvz_device(self._c_app)
        self._c_scene = sc.dvz_scene()
        self._c_fig = sc.dvz_figure(self._c_scene, 800, 600, 1, 1, 0)

        self._c_panel = sc.dvz_panel(
            self._c_fig, 0, 0, tp.DVZ_PANEL_TYPE_NONE, 0)
        self._c_visual = sc.dvz_visual(self._c_scene, tp.DVZ_VISUAL_POINT, 0)

        n = 50
        self._arr = np.zeros(n,
                             dtype=[('pos', 'f4', 3), ('color', 'u1', 4), ('size', 'f4')])
        t = np.linspace(-1, 1, n, dtype=np.float32)
        self._arr['pos'] = .75 * \
            np.c_[np.cos(np.pi*t), np.sin(np.pi*t), np.zeros(n)]
        self._arr['color'][:] = 255
        self._arr['size'][:] = 10.0
        cdef tp.uint8_t * data = <tp.uint8_t*> & self._arr.data[0]
        sc.dvz_visual_data(self._c_visual, tp.DVZ_PROP_NONE, 0, n, data)

        sc.dvz_panel_visual(self._c_panel, self._c_visual, 0)

    def set_data(self, np.ndarray[dtype=tp.uint8_t, ndim=2] color):
        n = 50
        self._arr['color'][:] = color
        cdef tp.uint8_t * data = <tp.uint8_t*> & self._arr.data[0]
        sc.dvz_visual_update(self._c_visual, tp.DVZ_PROP_NONE, 0, n, data)
        pt.dvz_device_update(self._c_device, self._c_scene)

    def run(self, sync=True):
        if sync:
            pt.dvz_device_run(self._c_device, self._c_scene, 0)
        else:
            pt.dvz_device_async(self._c_device, self._c_scene, 0)

    def wait(self):
        pt.dvz_device_wait(self._c_device)

    def destroy(self):
        sc.dvz_figure_destroy(self._c_fig)
        sc.dvz_scene_destroy(self._c_scene)

        pt.dvz_device_destroy(self._c_device)
        pt.dvz_app_destroy(self._c_app)


def proto(sync=True):
    p = Proto()
    p.run(sync=sync)
    if sync:
        p.destroy()
    return p


if __name__ == '__main__':
    p = proto()

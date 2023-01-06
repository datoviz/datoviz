# cython: c_string_type=unicode, c_string_encoding=ascii

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

# from .renderer cimport Renderer
from . cimport _types as tp
from . cimport app as pt
from . cimport request as rq
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
# App
# -------------------------------------------------------------------------------------------------

cdef class App:
    cdef pt.DvzApp * _c_app
    cdef pt.DvzDevice * _c_device
    cdef rq.DvzRequester * _c_rqr

    cdef np.ndarray _arr

    def __init__(self):
        self._c_app = pt.dvz_app(pt.DVZ_BACKEND_GLFW)
        self._c_device = pt.dvz_device(self._c_app)
        self._c_rqr = rq.dvz_requester()

    def run(self, sync=True):
        if sync:
            pt.dvz_device_run(self._c_device, self._c_rqr, 0)
        else:
            pt.dvz_device_async(self._c_device, self._c_rqr, 0)

    def wait(self):
        pt.dvz_device_wait(self._c_device)

    def destroy(self):
        rq.dvz_requester_destroy(self._c_rqr)
        pt.dvz_device_destroy(self._c_device)
        pt.dvz_app_destroy(self._c_app)


def app(sync=True):
    p = App()
    p.run(sync=sync)
    if sync:
        p.destroy()
    return p


if __name__ == '__main__':
    p = app()

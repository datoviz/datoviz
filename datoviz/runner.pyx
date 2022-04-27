# cython: c_string_type=unicode, c_string_encoding=ascii

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import logging
from contextlib import contextmanager

cimport numpy as np
import numpy as np
from cython.view cimport array
from libc.stdlib cimport free

from . cimport runner as rn
from . cimport requester as rq
from . cimport _types as tp

from .runner cimport Runner

from .requester cimport Request, Requester
from .renderer cimport Renderer


logger = logging.getLogger('datoviz')


# -------------------------------------------------------------------------------------------------
# Renderer
# -------------------------------------------------------------------------------------------------

cdef class Runner:
    def __cinit__(self, Renderer renderer):
        self._c_runner = rn.dvz_runner(renderer._c_rd)
        assert self._c_runner != NULL

    def __dealloc__(self):
        self.destroy()

    def destroy(self):
        """Destroy the runner."""
        # rn.dvz_runner_destroy(self._c_runner)

    def requester(self, Requester requester):
        rn.dvz_runner_requester(self._c_runner, &requester._c_rqr)

    def frame(self):
        rn.dvz_runner_frame(self._c_runner)

    def run(self, frame_count=0):
        rn.dvz_runner_loop(self._c_runner, frame_count)

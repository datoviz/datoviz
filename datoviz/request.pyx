# cython: c_string_type=unicode, c_string_encoding=ascii

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from . cimport cydatoviz as cv
from libc.stdio cimport printf
from libc.string cimport memcpy
from cpython.ref cimport Py_INCREF
import numpy as np
from functools import wraps, partial
import logging
import traceback
import sys

cimport numpy as np


logger = logging.getLogger('datoviz')


# -------------------------------------------------------------------------------------------------
# Types
# -------------------------------------------------------------------------------------------------

ctypedef np.float32_t FLOAT
ctypedef np.double_t DOUBLE
ctypedef np.uint8_t CHAR
ctypedef np.uint8_t[4] CVEC4
ctypedef np.int16_t SHORT
ctypedef np.uint16_t USHORT
ctypedef np.int32_t INT
ctypedef np.uint32_t UINT
ctypedef np.uint32_t[3] TEX_SHAPE


# -------------------------------------------------------------------------------------------------
# Constants
# -------------------------------------------------------------------------------------------------
# region  # folding in VSCode

DEFAULT_WIDTH = 800
DEFAULT_HEIGHT = 600

# cdef TEX_SHAPE DVZ_ZERO_OFFSET = (0, 0, 0)

# endregion


# -------------------------------------------------------------------------------------------------
# Constant utils
# -------------------------------------------------------------------------------------------------


# -------------------------------------------------------------------------------------------------
# Python event callbacks
# -------------------------------------------------------------------------------------------------


# -------------------------------------------------------------------------------------------------
# Public functions
# -------------------------------------------------------------------------------------------------


# -------------------------------------------------------------------------------------------------
# Util functions
# -------------------------------------------------------------------------------------------------


# -------------------------------------------------------------------------------------------------
# Requester
# -------------------------------------------------------------------------------------------------

cdef class Requester:
    """Singleton object that gives access to the GPUs."""

    cdef cv.DvzRequester _c_rqr

    def __cinit__(self):
        self._c_rqr = cv.dvz_requester()

    def __dealloc__(self):
        self.destroy()

    def destroy(self):
        """Destroy the requester."""
        cv.dvz_requester_destroy(&self._c_rqr)

    def create_board(self, int width, int height):
        cdef cv.DvzRequest req = cv.dvz_create_board(&self._c_rqr, width, height, 0)
        cv.dvz_request_print(&req)

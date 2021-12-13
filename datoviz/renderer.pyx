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

from . cimport renderer as rd
from .renderer cimport Renderer
from . cimport request as rq
from . cimport fileio
from . cimport _types as tp


logger = logging.getLogger('datoviz')


# -------------------------------------------------------------------------------------------------
# Renderer
# -------------------------------------------------------------------------------------------------

cdef class Renderer:
    # cdef rd.DvzRenderer* _c_rd
    # cdef rd.DvzGpu* _c_gpu

    def __cinit__(self):
        self._c_gpu = rd.dvz_init_offscreen()
        assert self._c_gpu != NULL
        self._c_rd = rd.dvz_renderer_offscreen(self._c_gpu)

    def __dealloc__(self):
        self.destroy()

    def destroy(self):
        """Destroy the renderer."""
        rd.dvz_renderer_destroy(self._c_rd)
        rd.dvz_host_destroy(self._c_gpu.host)

    def save_image(self, DvzId board, str path):
        cdef DvzSize size = 0
        cdef rd.DvzBoard* _board = rd.dvz_renderer_board(self._c_rd, board)
        cdef uint8_t* rgb = rd.dvz_renderer_image(self._c_rd, board, &size, NULL)
        fileio.dvz_write_png(path, _board.width, _board.height, rgb)

    def get_image(self, DvzId board):
        cdef rd.DvzBoard* _board = rd.dvz_renderer_board(self._c_rd, board)

        cdef DvzSize size = 0
        cdef np.ndarray arr
        arr = np.empty((_board.height, _board.width, 3), dtype=np.uint8)
        cdef uint8_t* pointer = <uint8_t*>&arr.data[0]
        rd.dvz_renderer_image(self._c_rd, board, &size, pointer)
        return arr

    def get_png(self, DvzId board):
        cdef np.ndarray arr
        arr = self.get_image(board)
        height, width = arr.shape[:2]
        cdef uint8_t* pointer = <uint8_t*>&arr.data[0]

        cdef DvzSize size = 0;
        cdef char* out = NULL;
        fileio.dvz_make_png(width, height, pointer, &size, <void**>&out);
        assert out != NULL
        assert size > 0

        ret = array(shape=(size,), itemsize=1, format='b', allocate_buffer=False)
        ret.data = out
        ret.callback_free_data = free

        return ret

# cython: c_string_type=unicode, c_string_encoding=ascii

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import logging

cimport numpy as np
import numpy as np

from . cimport renderer as rd
from . cimport request as rq
from . cimport fileio


logger = logging.getLogger('datoviz')


# -------------------------------------------------------------------------------------------------
# Renderer
# -------------------------------------------------------------------------------------------------

cdef class Renderer:
    cdef rd.DvzRenderer* _c_rd
    cdef rq.DvzRequester _c_rqr
    cdef rd.DvzGpu* _c_gpu

    def __cinit__(self):
        self._c_gpu = rd.dvz_init_offscreen()
        assert self._c_gpu != NULL
        self._c_rd = rd.dvz_renderer_offscreen(self._c_gpu)

        self._c_rqr = rq.dvz_requester()

    def __dealloc__(self):
        self.destroy()

    def destroy(self):
        """Destroy the renderer."""
        rq.dvz_requester_destroy(&self._c_rqr)
        rd.dvz_renderer_destroy(self._c_rd)
        rd.dvz_host_destroy(self._c_gpu.host)

    def _create_board(self, int width, int height, int id=0):
        cdef rq.DvzRequest req = rq.dvz_create_board(&self._c_rqr, width, height, 0)
        if id != 0:
            req.id = id
        rd.dvz_renderer_request(self._c_rd, req);
        return req.id

    def _create_graphics(self, DvzId board_id, DvzGraphicsType type, int id=0):
        cdef rq.DvzRequest req = rq.dvz_create_graphics(&self._c_rqr, board_id, type, 0);
        if id != 0:
            req.id = id
        rd.dvz_renderer_request(self._c_rd, req);
        return req.id

    def _create_dat(self, DvzBufferType type, DvzSize size, int id=0):
        cdef rq.DvzRequest req = rq.dvz_create_dat(&self._c_rqr, type, size, 0);
        if id != 0:
            req.id = id
        rd.dvz_renderer_request(self._c_rd, req);
        return req.id

    def _set_vertex(self, DvzId graphics, DvzId dat):
        cdef rq.DvzRequest req = rq.dvz_set_vertex(&self._c_rqr, graphics, dat);
        rd.dvz_renderer_request(self._c_rd, req);

    def _upload_dat(self, DvzId dat, DvzSize offset, np.ndarray data):
        cdef rq.DvzRequest req = rq.dvz_upload_dat(
            &self._c_rqr, dat, offset, data.size * data.itemsize, &data.data[0]);
        rd.dvz_renderer_request(self._c_rd, req);

    def _set_begin(self, DvzId board):
        cdef rq.DvzRequest req = rq.dvz_set_begin(&self._c_rqr, board);
        rd.dvz_renderer_request(self._c_rd, req);

    def _set_viewport(self, DvzId board, float x, float y, float w, float h):
        cdef vec2 offset = (x, y)
        cdef vec2 shape = (w, h)
        cdef rq.DvzRequest req = rq.dvz_set_viewport(&self._c_rqr, board, offset, shape);
        rd.dvz_renderer_request(self._c_rd, req);

    def _set_draw(self, DvzId board, DvzId graphics, int first_vertex, int vertex_count):
        cdef rq.DvzRequest req = rq.dvz_set_draw(&self._c_rqr, board, graphics, first_vertex, vertex_count);
        rd.dvz_renderer_request(self._c_rd, req);

    def _set_end(self, DvzId board):
        cdef rq.DvzRequest req = rq.dvz_set_end(&self._c_rqr, board);
        rd.dvz_renderer_request(self._c_rd, req);

    def _update_board(self, DvzId board):
        cdef rq.DvzRequest req = rq.dvz_update_board(&self._c_rqr, board);
        rd.dvz_renderer_request(self._c_rd, req);

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
        rd.dvz_renderer_image(self._c_rd, board, &size, &arr.data[0])
        return arr

# cython: c_string_type=unicode, c_string_encoding=ascii

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from . cimport renderer as rd
from . cimport request as rq
import logging


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

    def _create_board(self, int width, int height):
        cdef rq.DvzRequest req = rq.dvz_create_board(&self._c_rqr, width, height, 0)
        rd.dvz_renderer_request(self._c_rd, req);
        return req.id

    def _create_graphics(self, DvzId board_id, DvzGraphicsType type):
        cdef rq.DvzRequest req = rq.dvz_create_graphics(&self._c_rqr, board_id, type, 0);
        rd.dvz_renderer_request(self._c_rd, req);

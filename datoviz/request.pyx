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

from . cimport request as rq
from . cimport renderer as rd
from . cimport fileio
from . cimport _types as tp

from .renderer cimport Renderer


logger = logging.getLogger('datoviz')


# -------------------------------------------------------------------------------------------------
# Enums
# -------------------------------------------------------------------------------------------------

_ACTIONS = {
    DVZ_REQUEST_ACTION_CREATE: 'create',
    DVZ_REQUEST_ACTION_DELETE: 'delete',
    DVZ_REQUEST_ACTION_RESIZE: 'resize',
    DVZ_REQUEST_ACTION_UPDATE: 'update',
    DVZ_REQUEST_ACTION_UPLOAD: 'upload',
    DVZ_REQUEST_ACTION_UPFILL: 'upfill',
    DVZ_REQUEST_ACTION_DOWNLOAD: 'download',
    DVZ_REQUEST_ACTION_SET: 'set',
    DVZ_REQUEST_ACTION_GET: 'get',
}

_OBJECTS = {
    DVZ_REQUEST_OBJECT_BOARD: 'board',
    DVZ_REQUEST_OBJECT_CANVAS: 'canvas',
    DVZ_REQUEST_OBJECT_DAT: 'dat',
    DVZ_REQUEST_OBJECT_TEX: 'tex',
    DVZ_REQUEST_OBJECT_SAMPLER: 'sampler',
    DVZ_REQUEST_OBJECT_COMPUTE: 'compute',
    DVZ_REQUEST_OBJECT_GRAPHICS: 'graphics',
    DVZ_REQUEST_OBJECT_BEGIN: 'begin',
    DVZ_REQUEST_OBJECT_VIEWPORT: 'viewport',
    DVZ_REQUEST_OBJECT_VERTEX: 'vertex',
    DVZ_REQUEST_OBJECT_BARRIER: 'barrier',
    DVZ_REQUEST_OBJECT_DRAW: 'draw',
    DVZ_REQUEST_OBJECT_END: 'end',
}


# -------------------------------------------------------------------------------------------------
# Request
# -------------------------------------------------------------------------------------------------

cdef class Request:
    cdef rq.DvzRequest _c_req

    def __cinit__(self, rq.DvzRequest req):
        self._c_req = req

    def __repr__(self):
        action = _ACTIONS.get(self._c_req.action, 'undefined')
        obj = _OBJECTS.get(self._c_req.type, 'undefined')
        return f'<Request {action} {obj}>'


# -------------------------------------------------------------------------------------------------
# Requester
# -------------------------------------------------------------------------------------------------

cdef class Requester:
    cdef rq.DvzRequester _c_rqr
    cdef uint32_t _c_count
    cdef rq.DvzRequest* _c_rqs

    def __cinit__(self):
        self._c_rqr = rq.dvz_requester()

    def __dealloc__(self):
        self.destroy()

    def destroy(self):
        """Destroy the requester."""
        rq.dvz_requester_destroy(&self._c_rqr)

    @contextmanager
    def requests(self):
        rq.dvz_requester_begin(&self._c_rqr)
        yield
        self._c_rqs = rq.dvz_requester_end(&self._c_rqr, &self._c_count)

    def submit(self, Renderer renderer):
        rd.dvz_renderer_requests(renderer._c_rd, self._c_count, self._c_rqs)

    def create_board(self, int width, int height, int id=0):
        cdef rq.DvzRequest req = rq.dvz_create_board(&self._c_rqr, width, height, 0)
        if id != 0:
            req.id = id
        rq.dvz_requester_add(&self._c_rqr, req)
        return req.id

    def create_graphics(self, DvzId board_id, DvzGraphicsType type, int id=0):
        cdef rq.DvzRequest req = rq.dvz_create_graphics(&self._c_rqr, board_id, type, 0);
        if id != 0:
            req.id = id
        rq.dvz_requester_add(&self._c_rqr, req)
        return req.id

    def create_dat(self, DvzBufferType type, DvzSize size, int id=0):
        cdef rq.DvzRequest req = rq.dvz_create_dat(&self._c_rqr, type, size, 0);
        if id != 0:
            req.id = id
        rq.dvz_requester_add(&self._c_rqr, req)
        return req.id

    def set_vertex(self, DvzId graphics, DvzId dat):
        cdef rq.DvzRequest req = rq.dvz_set_vertex(&self._c_rqr, graphics, dat);
        rq.dvz_requester_add(&self._c_rqr, req)

    def upload_dat(self, DvzId dat, DvzSize offset, np.ndarray data):
        cdef rq.DvzRequest req = rq.dvz_upload_dat(
            &self._c_rqr, dat, offset, data.size * data.itemsize, &data.data[0]);
        rq.dvz_requester_add(&self._c_rqr, req)

    def set_begin(self, DvzId board):
        cdef rq.DvzRequest req = rq.dvz_set_begin(&self._c_rqr, board);
        rq.dvz_requester_add(&self._c_rqr, req)

    def set_viewport(self, DvzId board, float x, float y, float w, float h):
        cdef vec2 offset = (x, y)
        cdef vec2 shape = (w, h)
        cdef rq.DvzRequest req = rq.dvz_set_viewport(&self._c_rqr, board, offset, shape);
        rq.dvz_requester_add(&self._c_rqr, req)

    def set_draw(self, DvzId board, DvzId graphics, int first_vertex, int vertex_count):
        cdef rq.DvzRequest req = rq.dvz_set_draw(&self._c_rqr, board, graphics, first_vertex, vertex_count);
        rq.dvz_requester_add(&self._c_rqr, req)

    def set_end(self, DvzId board):
        cdef rq.DvzRequest req = rq.dvz_set_end(&self._c_rqr, board);
        rq.dvz_requester_add(&self._c_rqr, req)

    def update_board(self, DvzId board):
        cdef rq.DvzRequest req = rq.dvz_update_board(&self._c_rqr, board);
        rq.dvz_requester_add(&self._c_rqr, req)

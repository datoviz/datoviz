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
    DVZ_REQUEST_ACTION_BIND: 'bind',
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

    # HACK: keep a reference of the arrays to be uploaded, to prevent them from being
    # collected by the garbage collector until they are effectively transferred to the GPU.
    cdef object _np_cache

    def __cinit__(self):
        self._c_rqr = rq.dvz_requester()

    def __init__(self):
        super(Requester, self).__init__()
        self._np_cache = []

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

        # HACK: we can clear the NumPy cache now that all pending requests have been processed.
        self._np_cache.clear()

    def create_board(self, int width, int height, int id=0, background=None, int flags=0):
        logger.debug(f"create board {width}x{height}, id={id}, flags={flags}")

        # Background color
        cdef cvec4 c_background
        if background is None:
            c_background[0] = 0
            c_background[1] = 8
            c_background[2] = 18
            c_background[3] = 255
        else:
            assert len(background) == 3
            c_background[0] = <uint8_t>int(background[0])
            c_background[1] = <uint8_t>int(background[1])
            c_background[2] = <uint8_t>int(background[2])
            c_background[3] = 255

        cdef rq.DvzRequest req = rq.dvz_create_board(&self._c_rqr, width, height, c_background, flags)
        if id != 0:
            req.id = id
        rq.dvz_requester_add(&self._c_rqr, req)
        return req.id

    def create_graphics(self, DvzId board_id, DvzGraphicsType type, int id=0, int flags=0):
        logger.debug(f"create graphics, type={type}, board={board_id}, id={id}, flags={flags}")
        cdef rq.DvzRequest req = rq.dvz_create_graphics(&self._c_rqr, board_id, type, flags);
        if id != 0:
            req.id = id
        rq.dvz_requester_add(&self._c_rqr, req)
        return req.id

    def create_dat(self, DvzBufferType type, DvzSize size, int id=0, int flags=0):
        logger.debug(f"create dat, type={type}, size={size}, id={id}, flags={flags}")
        cdef rq.DvzRequest req = rq.dvz_create_dat(&self._c_rqr, type, size, flags);
        if id != 0:
            req.id = id
        rq.dvz_requester_add(&self._c_rqr, req)
        return req.id

    def set_vertex(self, DvzId graphics, DvzId dat):
        logger.debug(f"set vertex, graphics={graphics}, dat={dat}")
        cdef rq.DvzRequest req = rq.dvz_set_vertex(&self._c_rqr, graphics, dat);
        rq.dvz_requester_add(&self._c_rqr, req)

    def bind_dat(self, DvzId pipe, int slot_idx, DvzId dat):
        logger.debug(f"bind dat, pipe={pipe}, slot_idx={slot_idx}, dat={dat}")
        cdef rq.DvzRequest req = rq.dvz_bind_dat(&self._c_rqr, pipe, slot_idx, dat);
        rq.dvz_requester_add(&self._c_rqr, req)

    def bind_tex(self, DvzId pipe, int slot_idx, DvzId tex, DvzId sampler):
        logger.debug(f"bind tex, pipe={pipe}, slot_idx={slot_idx}, tex={tex}, sampler={sampler}")
        cdef rq.DvzRequest req = rq.dvz_bind_tex(&self._c_rqr, pipe, slot_idx, tex, sampler);
        rq.dvz_requester_add(&self._c_rqr, req)

    def upload_dat(self, DvzId dat, DvzSize offset, np.ndarray data):
        logger.debug(f"upload dat, dat={dat}, offset={offset}, data=<array {data.dtype} with {str(data.size)} items>")

        # HACK: keep a reference of the NumPy array to prevent it from being collected
        self._np_cache.append(data)

        cdef rq.DvzRequest req = rq.dvz_upload_dat(
            &self._c_rqr, dat, offset, data.size * data.itemsize, &data.data[0]);
        rq.dvz_requester_add(&self._c_rqr, req)

    def record_begin(self, DvzId board):
        logger.debug(f"set begin, board={board}")
        cdef rq.DvzRequest req = rq.dvz_record_begin(&self._c_rqr, board);
        rq.dvz_requester_add(&self._c_rqr, req)

    def record_viewport(self, DvzId board, int x, int y, int w, int h):
        logger.debug(f"record viewport, board={board}, x={x}, y={y}, w={w}, h={h}")
        cdef vec2 offset = (x, y)
        cdef vec2 shape = (w, h)
        cdef rq.DvzRequest req = rq.dvz_record_viewport(&self._c_rqr, board, offset, shape);
        rq.dvz_requester_add(&self._c_rqr, req)

    def record_draw(self, DvzId board, DvzId graphics, int first_vertex, int vertex_count):
        logger.debug(f"record draw, board={board}, graphics={graphics}, first_vertex={first_vertex}, vertex_count={vertex_count}")
        cdef rq.DvzRequest req = rq.dvz_record_draw(&self._c_rqr, board, graphics, first_vertex, vertex_count);
        rq.dvz_requester_add(&self._c_rqr, req)

    def record_end(self, DvzId board):
        logger.debug(f"set end, board={board}")
        cdef rq.DvzRequest req = rq.dvz_record_end(&self._c_rqr, board);
        rq.dvz_requester_add(&self._c_rqr, req)

    def update_board(self, DvzId board):
        logger.debug(f"update board, board={board}")
        cdef rq.DvzRequest req = rq.dvz_update_board(&self._c_rqr, board);
        rq.dvz_requester_add(&self._c_rqr, req)

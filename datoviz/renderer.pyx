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
from . cimport request as rq
from . cimport fileio
from . cimport _types as tp


logger = logging.getLogger('datoviz')


# -------------------------------------------------------------------------------------------------
# Request
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

cdef class Request:
    cdef rq.DvzRequest _c_req

    def __cinit__(self, rq.DvzRequest req):
        self._c_req = req

    def __repr__(self):
        action = _ACTIONS.get(self._c_req.action, 'undefined')
        obj = _OBJECTS.get(self._c_req.type, 'undefined')
        return f'<Request {action} {obj}>'


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

    @contextmanager
    def requests(self):
        out = []
        rq.dvz_requester_begin(&self._c_rqr)
        yield out
        cdef uint32_t count = 0
        cdef rq.DvzRequest* requests = rq.dvz_requester_end(&self._c_rqr, &count)
        for i in range(count):
            out.append(Request(requests[i]))

    def _create_board(self, int width, int height, int id=0):
        cdef rq.DvzRequest req = rq.dvz_create_board(&self._c_rqr, width, height, 0)
        if id != 0:
            req.id = id
        rq.dvz_requester_add(&self._c_rqr, req)
        # rd.dvz_renderer_request(self._c_rd, req);
        return req.id

    def _create_graphics(self, DvzId board_id, DvzGraphicsType type, int id=0):
        cdef rq.DvzRequest req = rq.dvz_create_graphics(&self._c_rqr, board_id, type, 0);
        if id != 0:
            req.id = id
        rq.dvz_requester_add(&self._c_rqr, req)
        # rd.dvz_renderer_request(self._c_rd, req);
        return req.id

    def _create_dat(self, DvzBufferType type, DvzSize size, int id=0):
        cdef rq.DvzRequest req = rq.dvz_create_dat(&self._c_rqr, type, size, 0);
        if id != 0:
            req.id = id
        rq.dvz_requester_add(&self._c_rqr, req)
        # rd.dvz_renderer_request(self._c_rd, req);
        return req.id

    def _set_vertex(self, DvzId graphics, DvzId dat):
        cdef rq.DvzRequest req = rq.dvz_set_vertex(&self._c_rqr, graphics, dat);
        rq.dvz_requester_add(&self._c_rqr, req)
        # rd.dvz_renderer_request(self._c_rd, req);

    def _upload_dat(self, DvzId dat, DvzSize offset, np.ndarray data):
        cdef rq.DvzRequest req = rq.dvz_upload_dat(
            &self._c_rqr, dat, offset, data.size * data.itemsize, &data.data[0]);
        rq.dvz_requester_add(&self._c_rqr, req)
        # rd.dvz_renderer_request(self._c_rd, req);

    def _set_begin(self, DvzId board):
        cdef rq.DvzRequest req = rq.dvz_set_begin(&self._c_rqr, board);
        rq.dvz_requester_add(&self._c_rqr, req)
        # rd.dvz_renderer_request(self._c_rd, req);

    def _set_viewport(self, DvzId board, float x, float y, float w, float h):
        cdef vec2 offset = (x, y)
        cdef vec2 shape = (w, h)
        cdef rq.DvzRequest req = rq.dvz_set_viewport(&self._c_rqr, board, offset, shape);
        rq.dvz_requester_add(&self._c_rqr, req)
        # rd.dvz_renderer_request(self._c_rd, req);

    def _set_draw(self, DvzId board, DvzId graphics, int first_vertex, int vertex_count):
        cdef rq.DvzRequest req = rq.dvz_set_draw(&self._c_rqr, board, graphics, first_vertex, vertex_count);
        rq.dvz_requester_add(&self._c_rqr, req)
        # rd.dvz_renderer_request(self._c_rd, req);

    def _set_end(self, DvzId board):
        cdef rq.DvzRequest req = rq.dvz_set_end(&self._c_rqr, board);
        rq.dvz_requester_add(&self._c_rqr, req)
        # rd.dvz_renderer_request(self._c_rd, req);

    def _update_board(self, DvzId board):
        cdef rq.DvzRequest req = rq.dvz_update_board(&self._c_rqr, board);
        rq.dvz_requester_add(&self._c_rqr, req)
        # rd.dvz_renderer_request(self._c_rd, req);

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

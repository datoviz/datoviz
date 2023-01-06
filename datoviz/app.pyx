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

    @contextmanager
    def commands(self):
        cmd = Commands(self)
        yield cmd

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


# -------------------------------------------------------------------------------------------------
# Record
# -------------------------------------------------------------------------------------------------

cdef class Record:
    cdef rq.DvzRequester * _c_rqr
    cdef DvzId _canvas_id

    def __init__(self, Canvas canvas):
        self._c_rqr = canvas._c_rqr
        self._canvas_id = canvas._c_id

    def viewport(self, float x=0.0, float y=0.0, float w=0.0, float h=0.0):
        cdef vec2 offset = (x, y)
        cdef vec2 shape = (w, h)

        logger.debug(
            f"record viewport, canvas={self._canvas_id:02x}, x={x}, y={y}, w={w}, h={h}")

        cdef rq.DvzRequest req = rq.dvz_record_viewport(self._c_rqr, self._canvas_id, offset, shape)
        rq.dvz_requester_add(self._c_rqr, req)

    def draw(self, Graphics g, int first_vertex, int vertex_count):
        cdef DvzId graphics_id = g._c_id

        logger.debug(
            f"record draw, canvas={self._canvas_id:02x}, graphics={graphics_id:02x}, first_vertex={first_vertex}, vertex_count={vertex_count}")

        req = rq.dvz_record_draw(self._c_rqr, self._canvas_id, graphics_id, first_vertex, vertex_count)
        rq.dvz_requester_add(self._c_rqr, req)


# -------------------------------------------------------------------------------------------------
# Canvas
# -------------------------------------------------------------------------------------------------

cdef class Canvas:
    cdef rq.DvzRequester * _c_rqr
    cdef DvzId _c_id

    def __init__(self, Commands cmd, DvzId id):
        self._c_rqr = cmd._c_rqr
        self._c_id = id

    @contextmanager
    def record(self):
        cdef DvzId canvas_id = self._c_id

        cdef rq.DvzRequest req = rq.dvz_record_begin(self._c_rqr, canvas_id)
        rq.dvz_requester_add(self._c_rqr, req)

        yield Record(self)

        req = rq.dvz_record_end(self._c_rqr, canvas_id)
        rq.dvz_requester_add(self._c_rqr, req)


# -------------------------------------------------------------------------------------------------
# Dat
# -------------------------------------------------------------------------------------------------

cdef class Dat:
    cdef rq.DvzRequester * _c_rqr
    cdef DvzId _c_id

    def __init__(self, Commands cmd, DvzId id):
        self._c_rqr = cmd._c_rqr
        self._c_id = id


# -------------------------------------------------------------------------------------------------
# Graphics
# -------------------------------------------------------------------------------------------------

cdef class Graphics:
    cdef rq.DvzRequester * _c_rqr
    cdef DvzId _c_id

    def __init__(self, Commands cmd, DvzId id):
        self._c_rqr = cmd._c_rqr
        self._c_id = id

    def set_vertex_buffer(self, Dat vb):
        cdef DvzId graphics_id = self._c_id
        cdef DvzId dat_id = vb._c_id
        logger.debug(f"set vertex, graphics={graphics_id:02x}, dat={dat_id:02x}")
        cdef rq.DvzRequest req = rq.dvz_set_vertex(self._c_rqr, graphics_id, dat_id)
        rq.dvz_requester_add(self._c_rqr, req)


# -------------------------------------------------------------------------------------------------
# Commands
# -------------------------------------------------------------------------------------------------

cdef class Commands:
    cdef rq.DvzRequester * _c_rqr
    cdef dict __dict__

    def __init__(self, App app):
        super(Commands, self).__init__()
        self._c_rqr = app._c_rqr

        # HACK: keep a reference of the NumPy arrays to prevent them from being collected
        self._np_cache = []

    cdef _submit_req(self, rq.DvzRequest req, DvzId id=0):
        if id != 0:
            req.id = id
        rq.dvz_requester_add(self._c_rqr, req)
        return req.id

    def Canvas(self, int width, int height, int flags=0, DvzId id=0):
        # Background color
        cdef cvec4 c_background
        # if background is None:
        c_background[0] = 0
        c_background[1] = 8
        c_background[2] = 18
        c_background[3] = 255
        # else:
        #     assert len(background) == 3
        #     c_background[0] = <uint8_t>int(background[0])
        #     c_background[1] = <uint8_t>int(background[1])
        #     c_background[2] = <uint8_t>int(background[2])
        #     c_background[3] = 255

        cdef rq.DvzRequest req = rq.dvz_create_canvas(self._c_rqr, width, height, c_background, flags)
        id = self._submit_req(req, id)
        logger.debug(f"create canvas {width}x{height}, id={id:02x}, flags={flags}")
        return Canvas(self, id)

    def Graphics(self, DvzGraphicsType gtype, int flags=0, DvzId id=0):
        cdef rq.DvzRequest req = rq.dvz_create_graphics(self._c_rqr, gtype, flags)
        id = self._submit_req(req, id)
        logger.debug(f"create graphics, gtype={gtype}, id={id:02x}, flags={flags}")
        return Graphics(self, id)

    def Dat(self, DvzBufferType btype, DvzSize size, int flags=0, DvzId id=0):
        cdef rq.DvzRequest req = rq.dvz_create_dat(self._c_rqr, btype, size, flags)
        id = self._submit_req(req, id)
        logger.debug(f"create dat, btype={btype}, size={size}, id={id:02x}, flags={flags}")
        return Dat(self, id)

    def VertexBuffer(self, np.ndarray arr):

        # HACK: keep a reference of the NumPy array to prevent it from being collected
        self._np_cache.append(arr)

        cdef DvzSize size = arr.size * arr.itemsize
        assert size > 0
        cdef DvzSize offset = 0

        cdef Dat dat = self.Dat(DVZ_BUFFER_TYPE_VERTEX, size)
        cdef DvzId dat_id = dat._c_id

        logger.debug(
            f"upload dat {dat_id:02x}, offset={offset}, data=<array {arr.dtype} with {str(arr.size)} items>")

        cdef tp.uint8_t * data = <tp.uint8_t*> & arr.data[0]
        cdef rq.DvzRequest req = rq.dvz_upload_dat(
            self._c_rqr, dat_id, offset, size, data)
        self._submit_req(req)

        return dat


# -------------------------------------------------------------------------------------------------
# Functions
# -------------------------------------------------------------------------------------------------

# def app(sync=True):
#     p = App()
#     p.run(sync=sync)
#     if sync:
#         p.destroy()
#     return p


# -------------------------------------------------------------------------------------------------
# Entry-point
# -------------------------------------------------------------------------------------------------

def main():
    app = App()

    n = 50
    arr = np.zeros(n, dtype=[('pos', 'f4', 3), ('color', 'u1', 4), ('size', 'f4')])
    t = np.linspace(-1, 1, n, dtype=np.float32)
    arr['pos'] = .75 * \
        np.c_[np.cos(np.pi*t), np.sin(np.pi*t), np.zeros(n)]
    arr['color'][:] = 255
    arr['size'][:] = 10.0

    with app.commands() as cmd:
        c = cmd.Canvas(width=800, height=600)
        g = cmd.Graphics(1, flags=3)  # default MVP and viewport
        vb = cmd.VertexBuffer(arr)
        g.set_vertex_buffer(vb)

        with c.record() as r:
            r.viewport(0, 0, 0, 0)
            r.draw(g, 0, n)
    app.run()


if __name__ == '__main__':
    main()
